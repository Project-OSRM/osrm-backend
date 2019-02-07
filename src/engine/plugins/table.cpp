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

namespace
{

// where does this speed lie with respect to the min/max penalizable speeds
inline double stoppage_penalty(double speed, double min_penalty, double penalty_range)
{
    // You're so slow already you don't get a penalty
    if (speed < MINIMAL_ACCEL_DECEL_PENALIZABLE_SPEED)
        return 0;

    // Find where it is on the scale
    constexpr auto max =
        MAXIMAL_ACCEL_DECEL_PENALIZABLE_SPEED - MINIMAL_ACCEL_DECEL_PENALIZABLE_SPEED;
    auto ratio = (speed - MINIMAL_ACCEL_DECEL_PENALIZABLE_SPEED) / max;

    // You're faster than the max so you get the max
    if (ratio >= 1)
        return min_penalty + penalty_range;

    // You're in between so you get a linear combination
    return min_penalty + ratio * penalty_range;
}

} // namespace

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

    auto result_tables_pair = algorithms.ManyToManySearch(
        snapped_phantoms, params.sources, params.destinations, request_distance);

    if ((request_duration && result_tables_pair.first.empty()) ||
        (request_distance && result_tables_pair.second.empty()))
    {
        return Error("NoTable", "No table found", result);
    }

    std::vector<api::TableAPI::TableCellRef> estimated_pairs;
    const auto stoppage_penalty_range = params.max_stoppage_penalty - params.min_stoppage_penalty;

    // Scan table for null results - if any exist, replace with distance estimates
    if (params.fallback_speed != INVALID_FALLBACK_SPEED || params.scale_factor != 1)
    {
        for (std::size_t row = 0; row < num_sources; row++)
        {
            for (std::size_t column = 0; column < num_destinations; column++)
            {
                const auto &table_index = row * num_destinations + column;
                BOOST_ASSERT(table_index < result_tables_pair.first.size());
                // Add decel/accel penalty to account for stoppage at the start/end of trip
                if (stoppage_penalty_range >= 0 &&
                    result_tables_pair.first[table_index] != MAXIMAL_EDGE_DURATION)
                {
                    const auto &source =
                        snapped_phantoms[params.sources.empty() ? row : params.sources[row]];
                    const auto &destination =
                        snapped_phantoms[params.destinations.empty() ? column
                                                                     : params.destinations[column]];

                    // TODO: if phantom node is actual node, will the distance/duration be 0?

                    auto source_penalty =
                        stoppage_penalty(source.forward_distance / source.forward_duration,
                                         params.min_stoppage_penalty,
                                         stoppage_penalty_range);
                    auto dest_penalty = stoppage_penalty(destination.forward_distance /
                                                             destination.forward_duration,
                                                         params.min_stoppage_penalty,
                                                         stoppage_penalty_range);

                    result_tables_pair.first[table_index] += source_penalty + dest_penalty;
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

    api::TableAPI table_api{facade, params};
    table_api.MakeResponse(result_tables_pair, snapped_phantoms, estimated_pairs, result);

    return Status::Ok;
}
} // namespace plugins
} // namespace engine
} // namespace osrm
