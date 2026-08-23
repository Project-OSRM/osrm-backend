#ifndef ENGINE_API_NEAREST_API_HPP
#define ENGINE_API_NEAREST_API_HPP

#include "engine/api/base_api.hpp"
#include "engine/api/base_result.hpp"
#include "engine/api/nearest_parameters.hpp"

#include "engine/api/json_factory.hpp"
#include "engine/phantom_node.hpp"

#include <boost/assert.hpp>

#include <vector>

namespace osrm::engine::api
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
        BOOST_ASSERT(phantom_nodes.size() == parameters.coordinates.size());

        if (std::holds_alternative<flatbuffers::FlatBufferBuilder>(response))
        {
            auto &fb_result = std::get<flatbuffers::FlatBufferBuilder>(response);
            MakeResponse(phantom_nodes, fb_result);
        }
        else
        {
            auto &json_result = std::get<util::json::Object>(response);
            MakeResponse(phantom_nodes, json_result);
        }
    }

    void MakeResponse(const std::vector<std::vector<PhantomNodeWithDistance>> &phantom_nodes,
                      flatbuffers::FlatBufferBuilder &fb_result) const
    {
        auto data_timestamp = facade.GetTimestamp();
        std::optional<flatbuffers::Offset<flatbuffers::String>> data_version_string = std::nullopt;
        if (!data_timestamp.empty())
        {
            data_version_string = fb_result.CreateString(data_timestamp);
        }

        // Builds the flatbuffers Waypoint vector for a single coordinate's matches. Shared by
        // both the N == 1 flat-field path and the N >= 2 grouped path below.
        auto make_waypoints_vector =
            [this,
             &fb_result](const std::vector<PhantomNodeWithDistance> &nodes_for_coordinate)
            -> flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<fbresult::Waypoint>>>
        {
            std::vector<flatbuffers::Offset<fbresult::Waypoint>> waypoints;
            waypoints.resize(nodes_for_coordinate.size());
            std::transform(nodes_for_coordinate.begin(),
                           nodes_for_coordinate.end(),
                           waypoints.begin(),
                           [this, &fb_result](const PhantomNodeWithDistance &phantom_with_distance)
                           {
                               auto &phantom_node = phantom_with_distance.phantom_node;

                               auto node_values = MakeNodes(phantom_node);
                               fbresult::Uint64Pair nodes{node_values.first, node_values.second};

                               auto waypoint = MakeWaypoint(&fb_result, {phantom_node});
                               waypoint->add_nodes(&nodes);
                               return waypoint->Finish();
                           });
            return fb_result.CreateVector(waypoints);
        };

        flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<fbresult::Waypoint>>>
            waypoints_vector;
        flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<fbresult::WaypointGroup>>>
            waypoints_grouped_vector;

        if (!parameters.skip_waypoints)
        {
            if (phantom_nodes.size() == 1)
            {
                // Single-coordinate case: unchanged flat field. HandleRequest already
                // returns NoSegment before reaching here if this coordinate is unmatched,
                // so phantom_nodes.front() is guaranteed non-empty at this point.
                waypoints_vector = make_waypoints_vector(phantom_nodes.front());
            }
            else
            {
                // Multi-coordinate case: one WaypointGroup per input coordinate, in order.
                // An unmatched coordinate carries `matched = false` and an Error instead of
                // failing the whole request.
                std::vector<flatbuffers::Offset<fbresult::WaypointGroup>> groups;
                groups.resize(phantom_nodes.size());
                std::transform(
                    phantom_nodes.begin(),
                    phantom_nodes.end(),
                    groups.begin(),
                    [this, &fb_result, &make_waypoints_vector](
                        const std::vector<PhantomNodeWithDistance> &nodes_for_coordinate)
                    {
                        fbresult::WaypointGroupBuilder group_builder(fb_result);
                        if (nodes_for_coordinate.empty())
                        {
                            auto message = fb_result.CreateString(
                                "Could not find a matching segment for coordinate");
                            auto code = fb_result.CreateString("NoSegment");
                            fbresult::ErrorBuilder error_builder(fb_result);
                            error_builder.add_code(code);
                            error_builder.add_message(message);
                            auto error = error_builder.Finish();

                            group_builder.add_matched(false);
                            group_builder.add_error(error);
                        }
                        else
                        {
                            auto waypoints = make_waypoints_vector(nodes_for_coordinate);
                            group_builder.add_matched(true);
                            group_builder.add_waypoints(waypoints);
                        }
                        return group_builder.Finish();
                    });
                waypoints_grouped_vector = fb_result.CreateVector(groups);
            }
        }

        fbresult::FBResultBuilder response(fb_result);

        if (phantom_nodes.size() == 1)
        {
            response.add_waypoints(waypoints_vector);
        }
        else
        {
            response.add_waypoints_grouped(waypoints_grouped_vector);
        }
        if (data_version_string)
        {
            response.add_data_version(*data_version_string);
        }
        fb_result.Finish(response.Finish());
    }

    void MakeResponse(const std::vector<std::vector<PhantomNodeWithDistance>> &phantom_nodes,
                      util::json::Object &response) const
    {
        auto make_waypoint_array = [this](const std::vector<PhantomNodeWithDistance> &nodes_for_coordinate)
        {
            util::json::Array waypoints;
            waypoints.values.resize(nodes_for_coordinate.size());
            std::transform(nodes_for_coordinate.begin(),
                           nodes_for_coordinate.end(),
                           waypoints.values.begin(),
                           [this](const PhantomNodeWithDistance &phantom_with_distance)
                           {
                               auto &phantom_node = phantom_with_distance.phantom_node;
                               auto waypoint = MakeWaypoint({phantom_node});

                               util::json::Array nodes;
                               nodes.values.reserve(2);

                               auto node_values = MakeNodes(phantom_node);

                               nodes.values.emplace_back(node_values.first);
                               nodes.values.emplace_back(node_values.second);
                               waypoint.values.emplace("nodes", std::move(nodes));
                               return waypoint;
                           });
            return waypoints;
        };

        if (!parameters.skip_waypoints)
        {
            if (phantom_nodes.size() == 1)
            {
                // Single-coordinate case: unchanged flat field. HandleRequest already
                // returns NoSegment before reaching here if this coordinate is unmatched,
                // so phantom_nodes.front() is guaranteed non-empty at this point.
                response.values.emplace("waypoints", make_waypoint_array(phantom_nodes.front()));
            }
            else
            {
                // Multi-coordinate case: independent per-coordinate results
                util::json::Array grouped;
                grouped.values.resize(phantom_nodes.size());
                for (std::size_t i = 0; i < phantom_nodes.size(); ++i)
                {
                    if (phantom_nodes[i].empty())
                    {
                        util::json::Object unmatched;
                        unmatched.values.emplace("code", "NoSegment");
                        unmatched.values.emplace(
                            "message", "Could not find a matching segment for coordinate");
                        grouped.values[i] = std::move(unmatched);
                    }
                    else
                    {
                        grouped.values[i] = make_waypoint_array(phantom_nodes[i]);
                    }
                }
                response.values.emplace("waypoints", std::move(grouped));
            }
        }

        response.values.emplace("code", "Ok");
        auto data_timestamp = facade.GetTimestamp();
        if (!data_timestamp.empty())
        {
            response.values.emplace("data_version", data_timestamp);
        }
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
                facade.GetOSMNodeIDOfNode(forward_geometry[phantom_node.fwd_segment_position]);
            to_node = static_cast<std::uint64_t>(osm_node_id);
        }

        if (phantom_node.reverse_segment_id.enabled)
        {
            auto segment_id = phantom_node.reverse_segment_id.id;
            const auto geometry_id = facade.GetGeometryIndex(segment_id).id;
            const auto geometry = facade.GetUncompressedForwardGeometry(geometry_id);
            auto osm_node_id =
                facade.GetOSMNodeIDOfNode(geometry[phantom_node.fwd_segment_position + 1]);
            from_node = static_cast<std::uint64_t>(osm_node_id);
        }
        else if (phantom_node.forward_segment_id.enabled && phantom_node.fwd_segment_position > 0)
        {
            // In the case of one way, rely on forward segment only
            auto osm_node_id =
                facade.GetOSMNodeIDOfNode(forward_geometry[phantom_node.fwd_segment_position - 1]);
            from_node = static_cast<std::uint64_t>(osm_node_id);
        }
        else if (phantom_node.forward_segment_id.enabled && phantom_node.fwd_segment_position == 0)
        {
            // At the very beginning of a one-way forward geometry.
            // Segment runs from the first OSM node to the second OSM node.
            BOOST_ASSERT(forward_geometry.size() >= 2);
            from_node = to_node;
            auto osm_node_id =
                facade.GetOSMNodeIDOfNode(forward_geometry[phantom_node.fwd_segment_position + 1]);
            to_node = static_cast<std::uint64_t>(osm_node_id);
        }

        return std::make_pair(from_node, to_node);
    }
};

} // namespace osrm::engine::api

#endif // OSRM_ENGINE_API_NEAREST_API_HPP