#include "engine/plugins/table.hpp"

#include "engine/api/table_api.hpp"
#include "engine/api/table_parameters.hpp"
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

Status TablePlugin::HandleRequest(const datafacade::ContiguousInternalMemoryDataFacadeBase &facade,
                                  const RoutingAlgorithmsInterface &algorithms,
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

    auto snapped_phantoms = SnapPhantomNodes(GetPhantomNodes(facade, params));
    auto result_table =
        algorithms.ManyToManySearch(snapped_phantoms, params.sources, params.destinations);

    if (result_table.empty())
    {
        return Error("NoTable", "No table found", result);
    }

    api::TableAPI table_api{facade, params};
    table_api.MakeResponse(result_table, snapped_phantoms, result);

    return Status::Ok;
}
}
}
}
