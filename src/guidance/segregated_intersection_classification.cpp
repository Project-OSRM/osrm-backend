#include "guidance/segregated_intersection_classification.hpp"
#include "extractor/intersection/coordinate_extractor.hpp"
#include "extractor/node_based_graph_factory.hpp"

#include "util/coordinate_calculation.hpp"
#include "util/name_table.hpp"

namespace osrm
{
namespace guidance
{

namespace RoadPriorityClass = extractor::RoadPriorityClass;

struct EdgeInfo
{
    EdgeID edge;

    NodeID node;

    util::StringView name;

    bool reversed;

    // 0 - outgoing (forward), 1 - incoming (reverse), 2 - both outgoing and incoming
    int direction;

    extractor::ClassData road_class;

    NodeBasedEdgeClassification flags;

    struct LessName
    {
        bool operator()(EdgeInfo const &e1, EdgeInfo const &e2) const { return e1.name < e2.name; }
    };
};

std::unordered_set<EdgeID> findSegregatedNodes(const extractor::NodeBasedGraphFactory &factory,
                                               const util::NameTable &names)
{

    auto const &graph = factory.GetGraph();
    auto const &annotation = factory.GetAnnotationData();
    auto const &coordinates = factory.GetCoordinates();

    extractor::intersection::CoordinateExtractor coordExtractor(
        graph, factory.GetCompressedEdges(), factory.GetCoordinates());

    auto const get_edge_length = [&](NodeID from_node, EdgeID edgeID, NodeID to_node) {
        auto const geom = coordExtractor.GetCoordinatesAlongRoad(from_node, edgeID, false, to_node);
        double length = 0.0;
        for (size_t i = 1; i < geom.size(); ++i)
        {
            length += util::coordinate_calculation::haversineDistance(geom[i - 1], geom[i]);
        }
        return length;
    };

    // Returns an angle between edges from from_edge_id to to_edge_id
    auto const get_angle = [&](NodeID from_node, EdgeID from_edge_id, NodeID to_edge_id) {
        auto intersection_node = graph.GetTarget(from_edge_id);
        auto from_edge_id_outgoing = graph.FindEdge(intersection_node, from_node);
        auto to_node = graph.GetTarget(to_edge_id);
        auto const node_to =
            coordExtractor.GetCoordinateCloseToTurn(intersection_node, to_edge_id, false, to_node);
        auto const node_from = coordExtractor.GetCoordinateCloseToTurn(
            intersection_node, from_edge_id_outgoing, false, from_node);
        return util::coordinate_calculation::computeAngle(
            node_from, coordinates[intersection_node], node_to);
    };

    auto const get_edge_info = [&](EdgeID edge_id, NodeID node, auto const &edgeData) -> EdgeInfo {
        /// @todo Make string normalization/lowercase/trim for comparison ...

        auto const id = annotation[edgeData.annotation_data].name_id;
        BOOST_ASSERT(id != INVALID_NAMEID);
        auto const name = names.GetNameForID(id);

        return {edge_id,
                node,
                name,
                edgeData.reversed,
                edgeData.reversed ? 1 : 0,
                annotation[edgeData.annotation_data].classes,
                edgeData.flags};
    };

    auto is_bidirectional = [](auto flags) { return flags.is_split || (!flags.is_split && flags.forward && flags.backward); };

    auto isSegregated = [&](NodeID node1,
                            std::vector<EdgeInfo> v1,
                            std::vector<EdgeInfo> v2,
                            EdgeInfo const &current,
                            double edgeLength) {
        // Internal intersection edges must be short and cannot be a roundabout
        // TODO adjust length as needed with lamda
        if (edgeLength > 32.0f || current.flags.roundabout) {
            return false;
        }

        // Print information about angles at node 1 from all inbound edge to the current edge
        bool oneway_inbound = false;
        for (auto const &edge_from : v1) {
            // Get the inbound edge and edge data
            auto edge_inbound = graph.FindEdge(edge_from.node, node1);
            auto const &edge_inbound_data = graph.GetEdgeData(edge_inbound);
            if (!edge_inbound_data.reversed) {
                // Skip any inbound edges not oneway (i.e. skip bidirectional)
                // and link edge
                // and not a road
                if (is_bidirectional(edge_inbound_data.flags)
                        || edge_inbound_data.flags.road_classification.IsLinkClass()
                        || (edge_inbound_data.flags.road_classification.GetClass() > RoadPriorityClass::SIDE_RESIDENTIAL)) {
                    continue;
                }

                // Get the turn degree from the inbound edge to the current edge
                auto const turn_degree = get_angle(edge_from.node, edge_inbound, current.edge);
                // Skip if the inbound edge is not somewhat perpendicular to the current edge
                // TODO add turn degree lamda
                if (turn_degree > 150 && turn_degree < 210) {
                    continue;
                }

                // If we are here the edge is a candidate oneway inbound
                oneway_inbound = true;
                break;
            }
        }

        // Return false if no valid oneway inbound edge
        if (!oneway_inbound) {
            return false;
        }

        // Print information about angles at node 2 from the current edge to all outbound edges
        bool oneway_outbound = false;
        for (auto const &edge_to : v2)
        {
            if (!edge_to.reversed)
            {
                // Skip any outbound edges not oneway (i.e. skip bidirectional)
                // and link edge
                // and not a road
                if (is_bidirectional(edge_to.flags)
                        || edge_to.flags.road_classification.IsLinkClass()
                        || (edge_to.flags.road_classification.GetClass() > RoadPriorityClass::SIDE_RESIDENTIAL)) {
                    continue;
                }

                // Get the turn degree from the current edge to the outbound edge
                auto const turn_degree = get_angle(node1, current.edge, edge_to.edge);
                // Skip if the outbound edge is not somewhat perpendicular to the current edge
                // TODO add turn degree lamda
                if (turn_degree > 150 && turn_degree < 210) {
                    continue;
                }

                // If we are here the edge is a candidate oneway outbound
                oneway_outbound = true;
                break;
            }
        }

        // Return false if no valid oneway outbound edge
        if (!oneway_outbound) {
            return false;
        }

        // TODO - determine if we need to add name checks

        // TODO - do we need to check headings of the inbound and outbound
        // oneway edges

        // Assume this is an intersection internal edge
        return true;
    };

    auto const collect_edge_info_fn = [&](auto const &edges1, NodeID node2) {
        std::vector<EdgeInfo> info;

        for (auto const &e : edges1)
        {
            NodeID const target = graph.GetTarget(e);
            if (target == node2)
                continue;

            info.push_back(get_edge_info(e, target, graph.GetEdgeData(e)));
        }

        if (info.empty())
            return info;

        std::sort(info.begin(), info.end(), [](EdgeInfo const &e1, EdgeInfo const &e2) {
            return e1.node < e2.node;
        });

        // Merge equal infos with correct direction.
        auto curr = info.begin();
        auto next = curr;
        while (++next != info.end())
        {
            if (curr->node == next->node)
            {
                BOOST_ASSERT(curr->name == next->name);
                BOOST_ASSERT(curr->road_class == next->road_class);
                BOOST_ASSERT(curr->direction != next->direction);
                curr->direction = 2;
            }
            else
                curr = next;
        }

        info.erase(
            std::unique(info.begin(),
                        info.end(),
                        [](EdgeInfo const &e1, EdgeInfo const &e2) { return e1.node == e2.node; }),
            info.end());

        return info;
    };

    auto const isSegregatedFn = [&](EdgeID edgeID,
                                    auto const &edgeData,
                                    auto const &edges1,
                                    NodeID node1,
                                    auto const &edges2,
                                    NodeID node2,
                                    double edgeLength) {
        return isSegregated(node1,
                            collect_edge_info_fn(edges1, node2),
                            collect_edge_info_fn(edges2, node1),
                            get_edge_info(edgeID, node1, edgeData),
                            edgeLength);
    };

    std::unordered_set<EdgeID> segregated_edges;

    for (NodeID sourceID = 0; sourceID < graph.GetNumberOfNodes(); ++sourceID)
    {
        auto const sourceEdges = graph.GetAdjacentEdgeRange(sourceID);
        for (EdgeID edgeID : sourceEdges)
        {
            auto const &edgeData = graph.GetEdgeData(edgeID);

            if (edgeData.reversed)
                continue;

            NodeID const targetID = graph.GetTarget(edgeID);
            auto const targetEdges = graph.GetAdjacentEdgeRange(targetID);

            double const length = get_edge_length(sourceID, edgeID, targetID);
            if (isSegregatedFn(edgeID, edgeData, sourceEdges, sourceID, targetEdges, targetID, length))
                segregated_edges.insert(edgeID);
        }
    }

    return segregated_edges;
}

} // namespace guidance
} // namespace osrm
