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

    auto result_tables_pair = algorithms.ManyToManySearch(
        snapped_phantoms, params.sources, params.destinations, request_distance);

    if ((request_duration && result_tables_pair.first.empty()) ||
        (request_distance && result_tables_pair.second.empty()))
    {
        return Error("NoTable", "No table found", result);
    }

    std::vector<api::TableAPI::TableCellRef> estimated_pairs;

    // Scan table for null results - if any exist, replace with distance estimates
    if (params.fallback_speed != INVALID_FALLBACK_SPEED || params.scale_factor != 1)
    {
        for (std::size_t row = 0; row < num_sources; row++)
        {
            for (std::size_t column = 0; column < num_destinations; column++)
            {
                const auto &table_index = row * num_destinations + column;
                BOOST_ASSERT(table_index < result_tables_pair.first.size());
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
}
}
}
