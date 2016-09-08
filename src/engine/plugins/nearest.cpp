#include "engine/plugins/nearest.hpp"
#include "engine/api/nearest_api.hpp"
#include "engine/api/nearest_parameters.hpp"
#include "engine/phantom_node.hpp"
#include "util/integer_range.hpp"

#include <cstddef>
#include <string>

#include <boost/algorithm/clamp.hpp>
#include <boost/assert.hpp>

namespace osrm
{
namespace engine
{
namespace plugins
{

NearestPlugin::NearestPlugin(datafacade::BaseDataFacade &facade,
                             const int max_results_,
                             const int max_radius_)
    : BasePlugin{facade}, max_results{max_results_}, max_radius{max_radius_}
{
}

Status NearestPlugin::HandleRequest(const api::NearestParameters &params,
                                    util::json::Object &json_result)
{
    BOOST_ASSERT(params.IsValid());

    if (!CheckAllCoordinates(params.coordinates))
        return Error("InvalidOptions", "Coordinates are invalid", json_result);

    if (params.coordinates.size() != 1)
    {
        return Error("InvalidOptions", "Only one input coordinate is supported", json_result);
    }

    // Artificially set and / or limit the params.radiuses array for GetPhantomNodes.
    // This sets a limit on nearest queries with bearing and large number of results.

    // params are const (read-only) but we have to constrain the radiuses afterwards.
    // We want to keep the user-interface as is, so make a copy. Should be cheap.
    auto constraint_params = params;

    if (max_radius != -1)
    {
        const auto max_radius_m = static_cast<double>(max_radius);

        if (constraint_params.radiuses.empty())
            constraint_params.radiuses.push_back(boost::make_optional(max_radius_m));
        else if (constraint_params.radiuses[0])
            boost::algorithm::clamp(*constraint_params.radiuses[0], 0., max_radius_m);
    }

    if (max_results != -1)
    {
        constraint_params.number_of_results = boost::algorithm::clamp(params.number_of_results, 0, max_results);
    }

    auto phantom_nodes = GetPhantomNodes(constraint_params, constraint_params.number_of_results);

    if (phantom_nodes.front().size() == 0)
    {
        return Error("NoSegment", "Could not find a matching segments for coordinate", json_result);
    }
    BOOST_ASSERT(phantom_nodes.front().size() > 0);

    api::NearestAPI nearest_api(facade, constraint_params);
    nearest_api.MakeResponse(phantom_nodes, json_result);

    return Status::Ok;
}
}
}
}
