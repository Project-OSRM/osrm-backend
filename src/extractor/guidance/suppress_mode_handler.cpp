#include "extractor/guidance/constants.hpp"
#include "extractor/guidance/suppress_mode_handler.hpp"
#include "extractor/travel_mode.hpp"

#include <algorithm>
#include <boost/assert.hpp>

namespace osrm
{
namespace extractor
{
namespace guidance
{

SuppressModeHandler::SuppressModeHandler(const IntersectionGenerator &intersection_generator,
                                         const util::NodeBasedDynamicGraph &node_based_graph,
                                         const std::vector<QueryNode> &node_info_list,
                                         const util::NameTable &name_table,
                                         const SuffixTable &street_name_suffix_table)
    : IntersectionHandler(node_based_graph,
                          node_info_list,
                          name_table,
                          street_name_suffix_table,
                          intersection_generator)
{
}

// travel modes for which navigation should be suppressed
const SuppressModeHandler::SuppressModeListT constexpr SUPPRESS_MODE_LIST = {
    {TRAVEL_MODE_TRAIN, TRAVEL_MODE_FERRY}};

bool SuppressModeHandler::canProcess(const NodeID /*nid*/,
                                     const EdgeID via_eid,
                                     const Intersection &intersection) const
{
    // if the approach way is not on the suppression blacklist, and not all the exit ways share that mode,
    // there are no ways to suppress by this criteria
    const auto in_mode = node_based_graph.GetEdgeData(via_eid).travel_mode;
    const auto suppress_in_mode =
        std::find(begin(SUPPRESS_MODE_LIST), end(SUPPRESS_MODE_LIST), in_mode);

    auto Begin = begin(intersection) + 1;
    auto End = end(intersection);
    const auto all_share_mode = std::all_of(Begin, End, [this, &in_mode](const ConnectedRoad &way) {
        return node_based_graph.GetEdgeData(way.eid).travel_mode == in_mode;
    });

    return (suppress_in_mode != end(SUPPRESS_MODE_LIST)) && all_share_mode;
}

Intersection SuppressModeHandler::
operator()(const NodeID nid, const EdgeID via_eid, Intersection intersection) const
{
    const auto suppress_all = SuppressModeHandler::canProcess(nid, via_eid, intersection);
    if (suppress_all)
    {
        std::for_each(begin(intersection) + 1, end(intersection), [&](ConnectedRoad &out_way) {
            out_way.instruction = {TurnType::Suppressed, getTurnDirection(out_way.angle)};
        });
    }

    return intersection;
}
}
}
}
