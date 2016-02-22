#include "engine/plugins/nearest.hpp"
#include "engine/api/nearest_parameters.hpp"
#include "engine/api/nearest_api.hpp"
#include "engine/phantom_node.hpp"
#include "util/integer_range.hpp"

#include <cstddef>
#include <string>

#include <boost/assert.hpp>

namespace osrm
{
namespace engine
{
namespace plugins
{

NearestPlugin::NearestPlugin(datafacade::BaseDataFacade &facade) : BasePlugin{facade} {}

Status NearestPlugin::HandleRequest(const api::NearestParameters &params,
                                    util::json::Object &json_result)
{
    BOOST_ASSERT(params.IsValid());

    if (!CheckAllCoordinates(params.coordinates))
        return Error("InvalidOptions", "Coordinates are invalid", json_result);

    if (params.coordinates.size() != 1)
    {
        return Error("TooBig", "Only one input coordinate is supported", json_result);
    }

    auto phantom_nodes = GetPhantomNodes(params, params.number_of_results);

    if (phantom_nodes.front().size() == 0)
    {
        return Error("NoSegment", "Could not find a matching segments for coordinate", json_result);
    }
    BOOST_ASSERT(phantom_nodes.front().size() > 0);

    api::NearestAPI nearest_api(facade, params);
    nearest_api.MakeResponse(phantom_nodes, json_result);

    return Status::Ok;
}
}
}
}
