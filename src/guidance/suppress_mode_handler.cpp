#include "guidance/suppress_mode_handler.hpp"
#include "extractor/travel_mode.hpp"

#include <algorithm>
#include <iterator>

namespace osrm::guidance
{

SuppressModeHandler::SuppressModeHandler(
    const util::NodeBasedDynamicGraph &node_based_graph,
    const extractor::EdgeBasedNodeDataContainer &node_data_container,
    const std::vector<util::Coordinate> &coordinates,
    const extractor::CompressedEdgeContainer &compressed_geometries,
    const extractor::RestrictionMap &node_restriction_map,
    const std::unordered_set<NodeID> &barrier_nodes,
    const extractor::TurnLanesIndexedArray &turn_lanes_data,
    const extractor::NameTable &name_table,
    const extractor::SuffixTable &street_name_suffix_table)
    : IntersectionHandler(node_based_graph,
                          node_data_container,
                          coordinates,
                          compressed_geometries,
                          node_restriction_map,
                          barrier_nodes,
                          turn_lanes_data,
                          name_table,
                          street_name_suffix_table)
{
}

bool SuppressModeHandler::canProcess(const NodeID,
                                     const EdgeID via_eid,
                                     const Intersection &intersection) const
{
    using std::begin;
    using std::end;

    // travel modes for which navigation should be suppressed
    static const constexpr char suppressed[] = {extractor::TRAVEL_MODE_TRAIN,
                                                extractor::TRAVEL_MODE_FERRY};

    // If the approach way is not on the suppression blacklist, and not all the exit ways share that
    // mode, there are no ways to suppress by this criteria.
    const auto in_mode =
        node_data_container.GetAnnotation(node_based_graph.GetEdgeData(via_eid).annotation_data)
            .travel_mode;
    const auto suppress_in_mode = std::find(begin(suppressed), end(suppressed), in_mode);

    const auto first = begin(intersection);
    const auto last = end(intersection);

    const auto all_share_mode = std::all_of(
        first,
        last,
        [this, &in_mode](const auto &road)
        {
            return node_data_container
                       .GetAnnotation(node_based_graph.GetEdgeData(road.eid).annotation_data)
                       .travel_mode == in_mode;
        });

    return (suppress_in_mode != end(suppressed)) && all_share_mode;
}

Intersection
SuppressModeHandler::operator()(const NodeID, const EdgeID, Intersection intersection) const
{
    const auto first = begin(intersection);
    const auto last = end(intersection);

    std::for_each(first,
                  last,
                  [&](auto &road)
                  {
                      const auto modifier = road.instruction.direction_modifier;
                      // use NoTurn, to not even have it as an IntermediateIntersection
                      const auto type = TurnType::NoTurn;

                      road.instruction = {type, modifier};
                  });

    return intersection;
}
} // namespace osrm::guidance
