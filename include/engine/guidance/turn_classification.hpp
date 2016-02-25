#ifndef OSRM_GUIDANCE_TURN_CLASSIFICATION_HPP_
#define OSRM_GUIDANCE_TURN_CLASSIFICATION_HPP_

#include "engine/guidance/turn_instruction.hpp"
#include "engine/guidance/guidance_toolkit.hpp"

#include "util/typedefs.hpp"
#include "util/coordinate.hpp"
#include "util/node_based_graph.hpp"

#include "extractor/compressed_edge_container.hpp"
#include "extractor/query_node.hpp"

#include <algorithm>
#include <cstddef>
#include <vector>

namespace osrm
{
namespace engine
{
namespace guidance
{

struct TurnPossibility
{
    TurnPossibility(DiscreteAngle angle, EdgeID edge_id)
        : angle(std::move(angle)), edge_id(std::move(edge_id))
    {
    }

    TurnPossibility() : angle(0), edge_id(SPECIAL_EDGEID) {}

    DiscreteAngle angle;
    EdgeID edge_id;
};

inline std::vector<TurnPossibility>
classifyIntersection(NodeID nid,
                     const util::NodeBasedDynamicGraph &graph,
                     const extractor::CompressedEdgeContainer &compressed_geometries,
                     const std::vector<extractor::QueryNode> &query_nodes)
{

    std::vector<TurnPossibility> turns;

    if (graph.BeginEdges(nid) == graph.EndEdges(nid))
        return std::vector<TurnPossibility>();

    const EdgeID base_id = graph.BeginEdges(nid);
    const auto base_coordinate = getRepresentativeCoordinate(nid, graph.GetTarget(base_id), base_id,
                                                             graph.GetEdgeData(base_id).reversed,
                                                             compressed_geometries, query_nodes);
    const auto node_coordinate = util::Coordinate(query_nodes[nid].lon, query_nodes[nid].lat);

    // generate a list of all turn angles between a base edge, the node and a current edge
    for (const EdgeID eid : graph.GetAdjacentEdgeRange(nid))
    {
        const auto edge_coordinate = getRepresentativeCoordinate(
            nid, graph.GetTarget(eid), eid, false, compressed_geometries, query_nodes);

        double angle = util::coordinate_calculation::computeAngle(base_coordinate, node_coordinate,
                                                                  edge_coordinate);
        turns.emplace_back(discretizeAngle(angle), eid);
    }

    std::sort(turns.begin(), turns.end(),
              [](const TurnPossibility left, const TurnPossibility right)
              {
                  return left.angle < right.angle;
              });

    turns.push_back(turns.front()); // sentinel
    for (std::size_t turn_nr = 0; turn_nr + 1 < turns.size(); ++turn_nr)
    {
        turns[turn_nr].angle = (256 + static_cast<uint32_t>(turns[turn_nr + 1].angle) -
                                static_cast<uint32_t>(turns[turn_nr].angle)) %
                               256; // calculate the difference to the right
    }
    turns.pop_back(); // remove sentinel again

    // find largest:
    std::size_t best_id = 0;
    DiscreteAngle largest_turn_angle = turns.front().angle;
    for (std::size_t current_turn_id = 1; current_turn_id < turns.size(); ++current_turn_id)
    {
        if (turns[current_turn_id].angle > largest_turn_angle)
        {
            largest_turn_angle = turns[current_turn_id].angle;
            best_id = current_turn_id;
        }
    }

    // rotate all angles so the largest angle comes first
    std::rotate(turns.begin(), turns.begin() + best_id, turns.end());

    return turns;
}

} // namespace guidance
} // namespace engine
} // namespace osrm

#endif // OSRM_GUIDANCE_TURN_CLASSIFICATION_HPP_
