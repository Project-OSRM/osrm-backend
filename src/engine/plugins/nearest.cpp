#include "engine/plugins/nearest.hpp"
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
                                    util::json::Object &result)
{
    BOOST_ASSERT(params.IsValid());

    if (!CheckAllCoordinates(params.coordinates))
        return Error("invalid-options", "Coordinates are invalid", result);

    if (params.bearings.size() > 0 && params.coordinates.size() != params.bearings.size())
        return Error("invalid-options", "Number of bearings does not match number of coordinates",
                     result);

    const auto &input_bearings = params.bearings;
    auto number_of_results = static_cast<std::size_t>(params.number_of_results);

    /* TODO(daniel-j-h): bearing range?
    const int bearing = input_bearings.size() > 0 ? input_bearings.front().first : 0;
    const int range = input_bearings.size() > 0
                          ? (input_bearings.front().second ? *input_bearings.front().second : 10)
                          : 180;
    auto phantom_node_vector =
        facade.NearestPhantomNodes(params.coordinates.front(), number_of_results, bearing, range);

    if (phantom_node_vector.empty())
    {
        result.values["status_message"] =
            std::string("Could not find a matching segments for coordinate");
        return Status::NoSegment;
    }
    else
    {
        result.values["status_message"] = "Found nearest edge";
        if (number_of_results > 1)
        {
            util::json::Array results;

            auto vector_length = phantom_node_vector.size();
            for (const auto i :
                 util::irange<std::size_t>(0, std::min(number_of_results, vector_length)))
            {
                const auto &node = phantom_node_vector[i].phantom_node;
                util::json::Array json_coordinate;
                util::json::Object result;
                json_coordinate.values.push_back(node.location.lat / COORDINATE_PRECISION);
                json_coordinate.values.push_back(node.location.lon / COORDINATE_PRECISION);
                result.values["mapped coordinate"] = json_coordinate;
                result.values["name"] = facade.get_name_for_id(node.name_id);
                results.values.push_back(result);
            }
            result.values["results"] = results;
        }
        else
        {
            util::json::Array json_coordinate;
            json_coordinate.values.push_back(phantom_node_vector.front().phantom_node.location.lat /
                                             COORDINATE_PRECISION);
            json_coordinate.values.push_back(phantom_node_vector.front().phantom_node.location.lon /
                                             COORDINATE_PRECISION);
            result.values["mapped_coordinate"] = json_coordinate;
            result.values["name"] =
                facade.get_name_for_id(phantom_node_vector.front().phantom_node.name_id);
        }
    }
    */
    return Status::Ok;
}
}
}
}
