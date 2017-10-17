#include "engine/plugins/table.hpp"

#include "engine/api/table_api.hpp"
#include "engine/api/table_parameters.hpp"
#include "engine/api/table_result.hpp"
#include "engine/error.hpp"
#include "engine/routing_algorithms/many_to_many.hpp"
#include "engine/search_engine_data.hpp"
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

Status TablePlugin::TablePlugin::HandleRequest(const RoutingAlgorithmsInterface &algorithms,
                                               const api::TableParameters &params,
                                               util::json::Object &result) const
{
    auto maybe_result = TablePlugin::HandleRequest(algorithms, params);
    if (maybe_result)
    {
        result = api::json::toJSON(static_cast<api::TableResult>(maybe_result));
        return Status::Ok;
    }
    else
    {
        result = api::json::toJSON(static_cast<engine::Error>(maybe_result));
        return Status::Error;
    }
}

MaybeResult<api::TableResult>
TablePlugin::HandleRequest(const RoutingAlgorithmsInterface &algorithms,
                           const api::TableParameters &params) const
{
    if (!algorithms.HasManyToManySearch())
    {
        return engine::Error{
            ErrorCode::NOT_IMPLEMENTED,
            "Many to many search is not implemented for the chosen search algorithm."};
    }

    BOOST_ASSERT(params.IsValid());

    if (!CheckAllCoordinates(params.coordinates))
    {
        return engine::Error{ErrorCode::INVALID_OPTIONS, "Coordinates are invalid"};
    }

    if (params.bearings.size() > 0 && params.coordinates.size() != params.bearings.size())
    {
        return engine::Error{ErrorCode::INVALID_OPTIONS,
                             "Number of bearings does not match number of coordinates"};
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
        return engine::Error{ErrorCode::TOO_BIG, "Too many table coordinates"};
    }

    auto error = CheckAlgorithms(params, algorithms);
    if (error.code != ErrorCode::NO_ERROR)
        return error;

    const auto &facade = algorithms.GetFacade();
    auto phantom_nodes = GetPhantomNodes(facade, params);

    if (phantom_nodes.size() != params.coordinates.size())
    {
        return engine::Error{ErrorCode::NO_SEGMENT,
                             std::string("Could not find a matching segment for coordinate ") +
                                 std::to_string(phantom_nodes.size())};
    }

    auto snapped_phantoms = SnapPhantomNodes(phantom_nodes);
    auto result_table =
        algorithms.ManyToManySearch(snapped_phantoms, params.sources, params.destinations);

    if (result_table.empty())
    {
        return engine::Error{ErrorCode::NO_TABLE, "No table found"};
    }

    api::TableAPI table_api{facade, params};
    auto result = table_api.MakeResponse(result_table, snapped_phantoms);

    return result;
}
}
}
}
