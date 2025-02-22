#include "guidance/segregated_intersection_classification.hpp"
#include "extractor/intersection/coordinate_extractor.hpp"
#include "extractor/node_based_graph_factory.hpp"
#include "guidance/turn_instruction.hpp"

#include "util/coordinate_calculation.hpp"
#include <set>

namespace osrm::guidance
{

// Maximum length in meters of an internal intersection edge
constexpr auto INTERNAL_LENGTH_MAX = 32.0f;

// The lower and upper bound internal straight values
constexpr auto INTERNAL_STRAIGHT_LOWER_BOUND = 150.0;
constexpr auto INTERNAL_STRAIGHT_UPPER_BOUND = 210.0;

struct EdgeInfo
{
    EdgeID edge;

    NodeID node;

    std::string_view name;

    bool reversed;

    extractor::ClassData road_class;

    extractor::NodeBasedEdgeClassification flags;

    struct LessName
    {
        bool operator()(EdgeInfo const &e1, EdgeInfo const &e2) const { return e1.name < e2.name; }
    };
};

std::unordered_set<EdgeID> findSegregatedNodes(const extractor::NodeBasedGraphFactory &factory,
                                               const extractor::NameTable &names)
{
    auto const &graph = factory.GetGraph();
    auto const &annotation = factory.GetAnnotationData();
    auto const &coordinates = factory.GetCoordinates();

    extractor::intersection::CoordinateExtractor coordExtractor(
        graph, factory.GetCompressedEdges(), coordinates);

    auto const get_edge_length = [&](NodeID from_node, EdgeID edge_id, NodeID to_node)
    {
        auto const geom =
            coordExtractor.GetCoordinatesAlongRoad(from_node, edge_id, false, to_node);
        double length = 0.0;
        for (size_t i = 1; i < geom.size(); ++i)
        {
            length += util::coordinate_calculation::greatCircleDistance(geom[i - 1], geom[i]);
        }
        return length;
    };

    // Returns an angle between edges from from_edge_id to to_edge_id
    auto const get_angle = [&](NodeID from_node, EdgeID from_edge_id, EdgeID to_edge_id)
    {
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

    auto const get_edge_info = [&](EdgeID edge_id, NodeID node, auto const &edge_data) -> EdgeInfo
    {
        /// @todo Make string normalization/lowercase/trim for comparison ...

        auto const id = annotation[edge_data.annotation_data].name_id;
        BOOST_ASSERT(id != INVALID_NAMEID);
        auto const name = names.GetNameForID(id);
        return {edge_id,
                node,
                name,
                edge_data.reversed,
                annotation[edge_data.annotation_data].classes,
                edge_data.flags};
    };

    auto is_bidirectional = [](auto flags)
    { return flags.is_split || (!flags.is_split && flags.forward && flags.backward); };

    auto is_internal_straight = [](auto const turn_degree)
    {
        return (turn_degree > INTERNAL_STRAIGHT_LOWER_BOUND &&
                turn_degree < INTERNAL_STRAIGHT_UPPER_BOUND);
    };

    // Lambda to check if the turn set includes a right turn type
    const auto has_turn_right = [](std::set<guidance::DirectionModifier::Enum> &turn_types)
    {
        return turn_types.find(guidance::DirectionModifier::Right) != turn_types.end() ||
               turn_types.find(guidance::DirectionModifier::SharpRight) != turn_types.end();
    };
    // Lambda to check if the turn set includes a left turn type
    const auto has_turn_left = [](std::set<guidance::DirectionModifier::Enum> &turn_types)
    {
        return turn_types.find(guidance::DirectionModifier::Left) != turn_types.end() ||
               turn_types.find(guidance::DirectionModifier::SharpLeft) != turn_types.end();
    };

    auto isSegregated = [&](NodeID node1,
                            const std::vector<EdgeInfo> &v1,
                            const std::vector<EdgeInfo> &v2,
                            EdgeInfo const &current,
                            double edge_length)
    {
        // Internal intersection edges must be short and cannot be a roundabout.
        // Also they must be a road use (not footway, cycleway, etc.)
        // TODO - consider whether alleys, cul-de-sacs, and other road uses
        // are candidates to be marked as internal intersection edges.
        // TODO adjust length as needed with lambda
        if (edge_length > INTERNAL_LENGTH_MAX || current.flags.roundabout || current.flags.circular)
        {
            return false;
        }
        // Iterate through inbound edges and get turn degrees from driveable inbound
        // edges onto the candidate edge.
        bool oneway_inbound = false;
        std::set<guidance::DirectionModifier::Enum> incoming_turn_type;
        for (auto const &edge_from : v1)
        {
            // Get the inbound edge and edge data
            auto edge_inbound = graph.FindEdge(edge_from.node, node1);
            auto const &edge_inbound_data = graph.GetEdgeData(edge_inbound);
            if (!edge_inbound_data.reversed)
            {
                // Store the turn type of incoming driveable edges.
                incoming_turn_type.insert(guidance::getTurnDirection(
                    get_angle(edge_from.node, edge_inbound, current.edge)));

                // Skip any inbound edges not oneway (i.e. skip bidirectional)
                // and link edge
                // and not a road
                if (is_bidirectional(edge_inbound_data.flags) ||
                    edge_inbound_data.flags.road_classification.IsLinkClass() ||
                    (edge_inbound_data.flags.road_classification.GetClass() >
                     extractor::RoadPriorityClass::SIDE_RESIDENTIAL))
                {
                    continue;
                }
                // Get the turn degree from the inbound edge to the current edge
                // Skip if the inbound edge is not somewhat perpendicular to the current edge
                if (is_internal_straight(get_angle(edge_from.node, edge_inbound, current.edge)))
                {
                    continue;
                }
                // If we are here the edge is a candidate oneway inbound
                oneway_inbound = true;
            }
        }

        // Must have an inbound oneway, excluding edges that are nearly straight
        // turn type onto the directed edge.
        if (!oneway_inbound)
        {
            return false;
        }

        // Iterate through outbound edges and get turn degrees from the candidate
        // edge onto outbound driveable edges.
        bool oneway_outbound = false;
        std::set<guidance::DirectionModifier::Enum> outgoing_turn_type;
        for (auto const &edge_to : v2)
        {
            if (!edge_to.reversed)
            {
                // Store outgoing turn type for any driveable edges
                outgoing_turn_type.insert(
                    guidance::getTurnDirection(get_angle(node1, current.edge, edge_to.edge)));

                // Skip any outbound edges not oneway (i.e. skip bidirectional)
                // and link edge
                // and not a road
                if (is_bidirectional(edge_to.flags) ||
                    edge_to.flags.road_classification.IsLinkClass() ||
                    (edge_to.flags.road_classification.GetClass() >
                     extractor::RoadPriorityClass::SIDE_RESIDENTIAL))
                {
                    continue;
                }

                // Get the turn degree from the current edge to the outbound edge
                // Skip if the outbound edge is not somewhat perpendicular to the current edge
                if (is_internal_straight(get_angle(node1, current.edge, edge_to.edge)))
                {
                    continue;
                }

                // If we are here the edge is a candidate oneway outbound
                oneway_outbound = true;
            }
        }
        // Must have outbound oneway at end node (exclude edges that are nearly
        // straight turn from directed edge
        if (!oneway_outbound)
        {
            return false;
        }

        // A further rejection case is if there are incoming edges that
        // have "opposite" turn degrees than outgoing edges or if the outgoing
        // edges have opposing turn degrees.
        if ((has_turn_left(incoming_turn_type) && has_turn_right(outgoing_turn_type)) ||
            (has_turn_right(incoming_turn_type) && has_turn_left(outgoing_turn_type)) ||
            (has_turn_left(outgoing_turn_type) && has_turn_right(outgoing_turn_type)))
        {
            return false;
        }

        // TODO - determine if we need to add name checks or need to check headings
        // of the inbound and outbound oneway edges

        // Assume this is an intersection internal edge
        return true;
    };

    auto const collect_edge_info_fn = [&](auto const &edges1, NodeID node2)
    {
        std::vector<EdgeInfo> info;

        for (auto e : edges1)
        {
            NodeID const target = graph.GetTarget(e);
            if (target == node2)
                continue;

            info.push_back(get_edge_info(e, target, graph.GetEdgeData(e)));
        }

        if (info.empty())
            return info;

        std::sort(info.begin(),
                  info.end(),
                  [](EdgeInfo const &e1, EdgeInfo const &e2) { return e1.node < e2.node; });

        info.erase(std::unique(info.begin(),
                               info.end(),
                               [](EdgeInfo const &e1, EdgeInfo const &e2)
                               { return e1.node == e2.node; }),
                   info.end());

        return info;
    };

    auto const isSegregatedFn = [&](EdgeID edge_id,
                                    auto const &edge_data,
                                    auto const &edges1,
                                    NodeID node1,
                                    auto const &edges2,
                                    NodeID node2,
                                    double edge_length)
    {
        return isSegregated(node1,
                            collect_edge_info_fn(edges1, node2),
                            collect_edge_info_fn(edges2, node1),
                            get_edge_info(edge_id, node1, edge_data),
                            edge_length);
    };

    std::unordered_set<EdgeID> segregated_edges;

    for (NodeID source_id = 0; source_id < graph.GetNumberOfNodes(); ++source_id)
    {
        auto const source_edges = graph.GetAdjacentEdgeRange(source_id);
        for (EdgeID edge_id : source_edges)
        {
            auto const &edgeData = graph.GetEdgeData(edge_id);

            if (edgeData.reversed)
                continue;

            NodeID const target_id = graph.GetTarget(edge_id);
            auto const targetEdges = graph.GetAdjacentEdgeRange(target_id);

            double const length = get_edge_length(source_id, edge_id, target_id);
            if (isSegregatedFn(
                    edge_id, edgeData, source_edges, source_id, targetEdges, target_id, length))
                segregated_edges.insert(edge_id);
        }
    }

    return segregated_edges;
}

} // namespace osrm::guidance
