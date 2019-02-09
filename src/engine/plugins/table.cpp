#include "engine/plugins/table.hpp"

#include "engine/api/table_api.hpp"
#include "engine/api/table_parameters.hpp"
#include "engine/routing_algorithms/many_to_many.hpp"
#include "engine/search_engine_data.hpp"
#include "util/coordinate_calculation.hpp"
#include "util/json_container.hpp"
#include "util/string_util.hpp"

#include <cstdlib>

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include <boost/assert.hpp>

namespace osrm
{
namespace engine
{
namespace plugins
{

TablePlugin::TablePlugin(const int max_locations_distance_table)
    : max_locations_distance_table(max_locations_distance_table)
{
}

Status TablePlugin::HandleRequest(const RoutingAlgorithmsInterface &algorithms,
                                  const api::TableParameters &params,
                                  util::json::Object &result) const
{
    if (!algorithms.HasManyToManySearch())
    {
        return Error("NotImplemented",
                     "Many to many search is not implemented for the chosen search algorithm.",
                     result);
    }

    BOOST_ASSERT(params.IsValid());

    if (!CheckAllCoordinates(params.coordinates))
    {
        return Error("InvalidOptions", "Coordinates are invalid", result);
    }

    if (params.bearings.size() > 0 && params.coordinates.size() != params.bearings.size())
    {
        return Error(
            "InvalidOptions", "Number of bearings does not match number of coordinates", result);
    }

    // Empty sources or destinations means the user wants all of them included, respectively
    // The ManyToMany routing algorithm we dispatch to below already handles this perfectly.
    const auto num_sources =
        params.sources.empty() ? params.coordinates.size() : params.sources.size();
    const auto num_destinations =
        params.destinations.empty() ? params.coordinates.size() : params.destinations.size();

    if (max_locations_distance_table > 0 &&
        ((num_sources * num_destinations) >
         static_cast<std::size_t>(max_locations_distance_table * max_locations_distance_table)))
    {
        return Error("TooBig", "Too many table coordinates", result);
    }

    if (!CheckAlgorithms(params, algorithms, result))
        return Status::Error;

    const auto &facade = algorithms.GetFacade();
    auto phantom_nodes = GetPhantomNodes(facade, params);

    if (phantom_nodes.size() != params.coordinates.size())
    {
        return Error("NoSegment",
                     std::string("Could not find a matching segment for coordinate ") +
                         std::to_string(phantom_nodes.size()),
                     result);
    }

    auto snapped_phantoms = SnapPhantomNodes(phantom_nodes);

    bool request_distance = params.annotations & api::TableParameters::AnnotationsType::Distance;
    bool request_duration = params.annotations & api::TableParameters::AnnotationsType::Duration;

    constexpr bool always_calculate_distance = true;

    auto result_tables_pair = algorithms.ManyToManySearch(
        snapped_phantoms, params.sources, params.destinations, always_calculate_distance);

    if ((request_duration && result_tables_pair.first.empty()) ||
        (request_distance && result_tables_pair.second.empty()))
    {
        return Error("NoTable", "No table found", result);
    }

    std::vector<api::TableAPI::TableCellRef> estimated_pairs;

    // Adds some time to adjust for getting up to speed and slowing down to a stop
    // Returns a new `duration`
    auto adjust_for_startstop = [](const double &comfortable_acceleration,
                                   const EdgeDuration &duration,
                                   const EdgeDistance &distance) -> EdgeDuration {

        // Assume linear acceleration from 0 to velocity
        // auto comfortable_acceleration = 1.85; // m/s^2

        // Very short paths can end up with 0 duration.  That'll lead to a divide
        // by zero, so instead, we'll assume the travel speed is 10m/s (36km/h).
        // Typically, the distance is also short, so we're quibbling at tiny numbers
        // here, but tiny numbers is what this adjustment lambda is all about,
        // so we do try to be reasonable.
        const auto average_speed =
            duration == 0 ? 10 : distance /
                                     (duration / 10.); // duration is in deciseconds, we need m/sec

        // Using the equations of motion as a simple approximation, assuming constant acceleration
        // https://en.wikipedia.org/wiki/Equations_of_motion#Constant_translational_acceleration_in_a_straight_line
        const auto distance_to_full_speed =
            (average_speed * average_speed) / (2 * comfortable_acceleration);

        /*
                std::cout << "Comfortable accel is " << comfortable_acceleration << std::endl;
                std::cout << "Average speed is " << average_speed << " duration is " << duration
                          << std::endl;
                std::cout << "Distance is " << distance << " distance to full speed is "
                          << distance_to_full_speed << std::endl;
                          */

        if (distance_to_full_speed > distance / 2)
        {
            // std::cout << "Distance was too short, so only using half" << std::endl;
            const auto time_to_halfway = std::sqrt(distance / comfortable_acceleration);
            return (2 * time_to_halfway) * 10; // result is in deciseconds
        }
        else
        {
            // std::cout << "Distance was long, using cruising speed" << std::endl;
            const auto cruising_distance = distance - 2 * distance_to_full_speed;
            const auto cruising_time = cruising_distance / average_speed;
            const auto acceleration_time = average_speed / comfortable_acceleration;

            // std::cout << "Cruising distance is " << cruising_distance << std::endl;
            // std::cout << "Cruising time is " << cruising_time << std::endl;
            // std::cout << "Acceleration time is " << acceleration_time << std::endl;
            return (cruising_time + 2 * acceleration_time) * 10; // result is in deciseconds
        }
    };

    // Scan table for null results - if any exist, replace with distance estimates
    if (params.fallback_speed != INVALID_FALLBACK_SPEED || params.scale_factor != 1 ||
        (params.min_stoppage_penalty != INVALID_MINIMUM_STOPPAGE_PENALTY &&
         params.max_stoppage_penalty != INVALID_MAXIMUM_STOPPAGE_PENALTY))
    {
        for (std::size_t row = 0; row < num_sources; row++)
        {
            for (std::size_t column = 0; column < num_destinations; column++)
            {
                const auto &table_index = row * num_destinations + column;
                BOOST_ASSERT(table_index < result_tables_pair.first.size());
                // apply an accel/deceleration penalty to the duration
                if (result_tables_pair.first[table_index] != MAXIMAL_EDGE_DURATION && row != column)
                {
                    /*
                    const auto &source =
                        snapped_phantoms[params.sources.empty() ? row : params.sources[row]];
                    const auto &destination =
                        snapped_phantoms[params.destinations.empty() ? column
                                                                     : params.destinations[column]];
                    */
                    result_tables_pair.first[table_index] =
                        adjust_for_startstop(params.min_stoppage_penalty,
                                             result_tables_pair.first[table_index],
                                             result_tables_pair.second[table_index]);
                }
                // Estimate null results based on fallback_speed (if valid) and distance
                if (params.fallback_speed != INVALID_FALLBACK_SPEED && params.fallback_speed > 0 &&
                    result_tables_pair.first[table_index] == MAXIMAL_EDGE_DURATION)
                {
                    const auto &source =
                        snapped_phantoms[params.sources.empty() ? row : params.sources[row]];
                    const auto &destination =
                        snapped_phantoms[params.destinations.empty() ? column
                                                                     : params.destinations[column]];

                    auto distance_estimate =
                        params.fallback_coordinate_type ==
                                api::TableParameters::FallbackCoordinateType::Input
                            ? util::coordinate_calculation::fccApproximateDistance(
                                  source.input_location, destination.input_location)
                            : util::coordinate_calculation::fccApproximateDistance(
                                  source.location, destination.location);

                    result_tables_pair.first[table_index] =
                        distance_estimate / (double)params.fallback_speed;
                    if (!result_tables_pair.second.empty())
                    {
                        result_tables_pair.second[table_index] = distance_estimate;
                    }

                    estimated_pairs.emplace_back(row, column);
                }
                // Apply a scale factor to non-null result if requested
                if (params.scale_factor > 0 && params.scale_factor != 1 &&
                    result_tables_pair.first[table_index] != MAXIMAL_EDGE_DURATION &&
                    result_tables_pair.first[table_index] != 0)
                {
                    EdgeDuration diff =
                        MAXIMAL_EDGE_DURATION / result_tables_pair.first[table_index];

                    if (params.scale_factor >= diff)
                    {
                        result_tables_pair.first[table_index] = MAXIMAL_EDGE_DURATION - 1;
                    }
                    else
                    {
                        result_tables_pair.first[table_index] = std::lround(
                            result_tables_pair.first[table_index] * params.scale_factor);
                    }
                }
            }
        }
    }

    // If distances weren't requested, delete them from the result so they don't
    // get rendered.
    if (!request_distance)
    {
        std::vector<EdgeDistance> empty;
        result_tables_pair.second.swap(empty);
    }

    api::TableAPI table_api{facade, params};
    table_api.MakeResponse(result_tables_pair, snapped_phantoms, estimated_pairs, result);

    return Status::Ok;
}
} // namespace plugins
} // namespace engine
} // namespace osrm
