#include "extractor/guidance/suppress_mode_handler.hpp"
#include "extractor/travel_mode.hpp"

#include <algorithm>
#include <iterator>

namespace osrm
{
namespace extractor
{
namespace guidance
{

SuppressModeHandler::SuppressModeHandler(const IntersectionGenerator &intersection_generator,
                                         const util::NodeBasedDynamicGraph &node_based_graph,
                                         const std::vector<util::Coordinate> &coordinates,
                                         const util::NameTable &name_table,
                                         const SuffixTable &street_name_suffix_table)
    : IntersectionHandler(node_based_graph,
                          coordinates,
                          name_table,
                          street_name_suffix_table,
                          intersection_generator)
{
}

bool SuppressModeHandler::canProcess(const NodeID,
                                     const EdgeID via_eid,
                                     const Intersection &intersection) const
{
    using std::begin;
    using std::end;

    // travel modes for which navigation should be suppressed
    static const constexpr char suppressed[] = {TRAVEL_MODE_TRAIN, TRAVEL_MODE_FERRY};

    // If the approach way is not on the suppression blacklist, and not all the exit ways share that
    // mode, there are no ways to suppress by this criteria.
    const auto in_mode = node_based_graph.GetEdgeData(via_eid).travel_mode;
    const auto suppress_in_mode = std::find(begin(suppressed), end(suppressed), in_mode);

    const auto first = begin(intersection);
    const auto last = end(intersection);

    const auto all_share_mode = std::all_of(first, last, [this, &in_mode](const auto &road) {
        return node_based_graph.GetEdgeData(road.eid).travel_mode == in_mode;
    });

    return (suppress_in_mode != end(suppressed)) && all_share_mode;
}

Intersection SuppressModeHandler::
operator()(const NodeID, const EdgeID, Intersection intersection) const
{
    const auto first = begin(intersection);
    const auto last = end(intersection);

    std::for_each(first, last, [&](auto &road) {
        const auto modifier = road.instruction.direction_modifier;
        // use NoTurn, to not even have it as an IntermediateIntersection
        const auto type = TurnType::NoTurn;

        road.instruction = {type, modifier};
    });

    return intersection;
}
}
}
}
