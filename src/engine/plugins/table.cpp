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

    // Because of the Short Trip ETA adjustments below, we need distances every time
    const bool distances_required = request_distance || params.waypoint_acceleration_factor > 0.;

    auto result_tables_pair = algorithms.ManyToManySearch(
        snapped_phantoms, params.sources, params.destinations, distances_required);

    if ((request_duration && result_tables_pair.first.empty()) ||
        (request_distance && result_tables_pair.second.empty()))
    {
        return Error("NoTable", "No table found", result);
    }

    std::vector<api::TableAPI::TableCellRef> estimated_pairs;

    // Adds some time to adjust for getting up to speed and slowing down to a stop
    // Returns a new `duration`
    auto adjust_for_startstop = [&](const double &acceleration_alpha,
                                   const EdgeDuration &duration,
                                   const EdgeDistance &distance) -> EdgeDuration {

        // Very short paths can end up with 0 duration.  That'll lead to a divide
        // by zero, so instead, we'll assume the travel speed is 10m/s (36km/h).
        // Typically, the distance is also short, so we're quibbling at tiny numbers
        // here, but tiny numbers is what this adjustment lambda is all about,
        // so we do try to be reasonable.
        const auto average_speed =
            duration == 0 ? 10 : distance /
                                     (duration / 10.); // duration is in deciseconds, we need m/sec

        // The following reference has a nice model (equations 9 through 12)
        // for vehicle acceleration
        // https://fdotwww.blob.core.windows.net/sitefinity/docs/default-source/content/rail/publications/studies/safety/accelerationresearch.pdf?sfvrsn=716a4bb1_0
        // We solve euqation 10 for time to figure out how long it'll take
        // to get up to the desired speed

        // Because Equation 10 is asymptotic on v_des, we need to pick a target speed
        // that's slighly less so the equation can actually get there.  1m/s less than
        // target seems like a reasonable value to aim for
        const auto target_speed = std::max(average_speed - 1, 0.1);
        const auto initial_speed = 0.;

        // Equation 9
        const auto beta = acceleration_alpha / average_speed;

        // Equation 10 solved for time
        const auto time_to_full_speed = std::log( (average_speed - initial_speed) / (average_speed - target_speed) ) / beta;
        BOOST_ASSERT(time_to_full_speed >= 0);

        // Equation 11
        const auto distance_to_full_speed =
            average_speed * time_to_full_speed -
            average_speed * (1 - std::exp(-1 * beta * time_to_full_speed)) / beta;

        BOOST_ASSERT(distance_to_full_speed >= 0);

        if (distance_to_full_speed > distance / 2)
        {
            // Because equation 12 requires velocity at halfway, and
            // solving equation 11 for t requires a Lambert W function,
            // we'll approximate here by assuming constant acceleration
            // over distance_to_full_speed using s = ut + 1/2at^2
            const auto average_acceleration =
                2 * distance_to_full_speed / (time_to_full_speed * time_to_full_speed);
            const auto time_to_halfway = std::sqrt(distance / average_acceleration);
            BOOST_ASSERT(time_to_halfway >= 0);
            return (2 * time_to_halfway) * 10; // result is in deciseconds
        }
        else
        {
            const auto cruising_distance = distance - 2 * distance_to_full_speed;
            const auto cruising_time = cruising_distance / average_speed;
            BOOST_ASSERT(cruising_time >= 0);
            return (cruising_time + 2 * time_to_full_speed) * 10; // result is in deciseconds
        }
    };

    // Scan table for null results - if any exist, replace with distance estimates
    if (params.fallback_speed != INVALID_FALLBACK_SPEED || params.scale_factor != 1 ||
        params.waypoint_acceleration_factor != 0.)
    {
        for (std::size_t row = 0; row < num_sources; row++)
        {
            for (std::size_t column = 0; column < num_destinations; column++)
            {
                const auto &table_index = row * num_destinations + column;
                BOOST_ASSERT(table_index < result_tables_pair.first.size());
                // apply an accel/deceleration penalty to the duration
                if (result_tables_pair.first[table_index] != MAXIMAL_EDGE_DURATION &&
                    row != column && params.waypoint_acceleration_factor != 0.)
                {
                    result_tables_pair.first[table_index] =
                        adjust_for_startstop(params.waypoint_acceleration_factor,
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
}
}
}
