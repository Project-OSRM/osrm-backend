#ifndef ENGINE_API_NEAREST_API_HPP
#define ENGINE_API_NEAREST_API_HPP

#include "engine/api/base_api.hpp"
#include "engine/api/nearest_parameters.hpp"

#include "engine/api/json_factory.hpp"
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

    void MakeResponse(const std::vector<std::vector<PhantomNodeWithDistance>> &phantom_nodes,
                      osrm::engine::api::ResultT &response) const
    {
        BOOST_ASSERT(phantom_nodes.size() == 1);
        BOOST_ASSERT(parameters.coordinates.size() == 1);

        if (response.is<flatbuffers::FlatBufferBuilder>())
        {
            auto &fb_result = response.get<flatbuffers::FlatBufferBuilder>();
            MakeResponse(phantom_nodes, fb_result);
        }
        else
        {
            auto &json_result = response.get<util::json::Object>();
            MakeResponse(phantom_nodes, json_result);
        }
    }

    void MakeResponse(const std::vector<std::vector<PhantomNodeWithDistance>> &phantom_nodes,
                      flatbuffers::FlatBufferBuilder &fb_result) const
    {
        auto data_timestamp = facade.GetTimestamp();
        boost::optional<flatbuffers::Offset<flatbuffers::String>> data_version_string = boost::none;
        if (!data_timestamp.empty())
        {
            data_version_string = fb_result.CreateString(data_timestamp);
        }

        std::vector<flatbuffers::Offset<fbresult::Waypoint>> waypoints;
        waypoints.resize(phantom_nodes.front().size());
        std::transform(phantom_nodes.front().begin(),
                       phantom_nodes.front().end(),
                       waypoints.begin(),
                       [this, &fb_result](const PhantomNodeWithDistance &phantom_with_distance) {
                           auto &phantom_node = phantom_with_distance.phantom_node;

                           auto node_values = MakeNodes(phantom_node);
                           fbresult::Uint64Pair nodes{node_values.first, node_values.second};

                           auto waypoint = MakeWaypoint(fb_result, phantom_node);
                           waypoint.add_nodes(&nodes);

                           return waypoint.Finish();
                       });

        auto waypoints_vector = fb_result.CreateVector(waypoints);
        fbresult::FBResultBuilder response(fb_result);
        response.add_waypoints(waypoints_vector);
        if (data_version_string)
        {
            response.add_data_version(*data_version_string);
        }
        fb_result.Finish(response.Finish());
    }
    void MakeResponse(const std::vector<std::vector<PhantomNodeWithDistance>> &phantom_nodes,
                      util::json::Object &response) const
    {
        util::json::Array waypoints;
        waypoints.values.resize(phantom_nodes.front().size());
        std::transform(phantom_nodes.front().begin(),
                       phantom_nodes.front().end(),
                       waypoints.values.begin(),
                       [this](const PhantomNodeWithDistance &phantom_with_distance) {
                           auto &phantom_node = phantom_with_distance.phantom_node;
                           auto waypoint = MakeWaypoint(phantom_node);

                           util::json::Array nodes;

                           auto node_values = MakeNodes(phantom_node);

                           nodes.values.push_back(node_values.first);
                           nodes.values.push_back(node_values.second);
                           waypoint.values["nodes"] = std::move(nodes);

                           return waypoint;
                       });

        response.values["code"] = "Ok";
        response.values["waypoints"] = std::move(waypoints);
    }

    const NearestParameters &parameters;

  protected:
    std::pair<uint64_t, uint64_t> MakeNodes(const PhantomNode &phantom_node) const
    {
        std::uint64_t from_node = 0;
        std::uint64_t to_node = 0;

        datafacade::BaseDataFacade::NodeForwardRange forward_geometry;
        if (phantom_node.forward_segment_id.enabled)
        {
            auto segment_id = phantom_node.forward_segment_id.id;
            const auto geometry_id = facade.GetGeometryIndex(segment_id).id;
            forward_geometry = facade.GetUncompressedForwardGeometry(geometry_id);

            auto osm_node_id =
                facade.GetOSMNodeIDOfNode(forward_geometry(phantom_node.fwd_segment_position));
            to_node = static_cast<std::uint64_t>(osm_node_id);
        }

        if (phantom_node.reverse_segment_id.enabled)
        {
            auto segment_id = phantom_node.reverse_segment_id.id;
            const auto geometry_id = facade.GetGeometryIndex(segment_id).id;
            const auto geometry = facade.GetUncompressedForwardGeometry(geometry_id);
            auto osm_node_id =
                facade.GetOSMNodeIDOfNode(geometry(phantom_node.fwd_segment_position + 1));
            from_node = static_cast<std::uint64_t>(osm_node_id);
        }
        else if (phantom_node.forward_segment_id.enabled && phantom_node.fwd_segment_position > 0)
        {
            // In the case of one way, rely on forward segment only
            auto osm_node_id =
                facade.GetOSMNodeIDOfNode(forward_geometry(phantom_node.fwd_segment_position - 1));
            from_node = static_cast<std::uint64_t>(osm_node_id);
        }

        return std::make_pair(from_node, to_node);
    }
};

} // ns api
} // ns engine
} // ns osrm

#endif
