#ifndef ENGINE_API_NEAREST_API_HPP
#define ENGINE_API_NEAREST_API_HPP

#include "engine/api/base_api.hpp"
#include "engine/api/nearest_parameters.hpp"
#include "engine/api/nearest_result.hpp"

#include "engine/phantom_node.hpp"

#include <boost/assert.hpp>

#include <vector>

namespace osrm
{
namespace engine
{
namespace api
{

class NearestAPI final : public BaseAPI
{
  public:
    NearestAPI(const datafacade::BaseDataFacade &facade_, const NearestParameters &parameters_)
        : BaseAPI(facade_, parameters_), parameters(parameters_)
    {
    }

    NearestResult
    MakeResponse(const std::vector<std::vector<PhantomNodeWithDistance>> &phantom_nodes) const
    {
        BOOST_ASSERT(phantom_nodes.size() == 1);
        BOOST_ASSERT(parameters.coordinates.size() == 1);

        NearestResult result;
        result.waypoints.resize(phantom_nodes.size());
        std::transform(
            phantom_nodes.front().begin(),
            phantom_nodes.front().end(),
            result.waypoints.begin(),
            [this](const PhantomNodeWithDistance &phantom_with_distance) {
                auto &phantom_node = phantom_with_distance.phantom_node;

                util::json::Array nodes;

                std::uint64_t from_node = 0;
                std::uint64_t to_node = 0;

                std::vector<NodeID> forward_geometry;
                if (phantom_node.forward_segment_id.enabled)
                {
                    auto segment_id = phantom_node.forward_segment_id.id;
                    const auto geometry_id = facade.GetGeometryIndex(segment_id).id;
                    forward_geometry = facade.GetUncompressedForwardGeometry(geometry_id);

                    auto osm_node_id = facade.GetOSMNodeIDOfNode(
                        forward_geometry[phantom_node.fwd_segment_position]);
                    to_node = static_cast<std::uint64_t>(osm_node_id);
                }

                if (phantom_node.reverse_segment_id.enabled)
                {
                    auto segment_id = phantom_node.reverse_segment_id.id;
                    const auto geometry_id = facade.GetGeometryIndex(segment_id).id;
                    std::vector<NodeID> geometry =
                        facade.GetUncompressedForwardGeometry(geometry_id);
                    auto osm_node_id =
                        facade.GetOSMNodeIDOfNode(geometry[phantom_node.fwd_segment_position + 1]);
                    from_node = static_cast<std::uint64_t>(osm_node_id);
                }
                else if (phantom_node.forward_segment_id.enabled &&
                         phantom_node.fwd_segment_position > 0)
                {
                    // In the case of one way, rely on forward segment only
                    auto osm_node_id = facade.GetOSMNodeIDOfNode(
                        forward_geometry[phantom_node.fwd_segment_position - 1]);
                    from_node = static_cast<std::uint64_t>(osm_node_id);
                }

                auto name = facade.GetNameForID(
                    facade.GetNameIndex(phantom_with_distance.phantom_node.forward_segment_id.id));
                return Waypoint{phantom_with_distance.distance,
                                name.data(),
                                phantom_with_distance.phantom_node.location,
                                {{{from_node}, {to_node}}}};
            });

        return result;
    }

    const NearestParameters &parameters;
};

} // ns api
} // ns engine
} // ns osrm

#endif
