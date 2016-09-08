#include "engine/plugins/nearest.hpp"
#include "engine/api/nearest_api.hpp"
#include "engine/api/nearest_parameters.hpp"
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

NearestPlugin::NearestPlugin(datafacade::BaseDataFacade &facade,
                             const int max_results_,
                             const double max_radius_when_bearing_)
    : BasePlugin{facade, max_radius_when_bearing_}, max_results{max_results_}
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

    if (max_results != -1 && params.number_of_results > boost::numeric_cast<unsigned>(max_results))
    {
        return Error("TooBig",
                     "Too many results requested, maximum is " + std::to_string(max_results),
                     json_result);
    }

    if (!CheckAllRadiuses(params))
    {
        return Error("TooBig",
                     "When using a bearing filter, the maximum search radius is limited to " +
                         std::to_string(max_radius_when_bearings) + "m",
                     json_result);
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
