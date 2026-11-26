#ifndef MANY_TO_MANY_ROUTING_HPP
#define MANY_TO_MANY_ROUTING_HPP

#include "engine/algorithm.hpp"
#include "engine/datafacade.hpp"
#include "engine/phantom_node.hpp"
#include "engine/search_engine_data.hpp"

#include "util/typedefs.hpp"

#include <boost/assert.hpp>

#include <algorithm>
#include <tuple>
#include <utility>
#include <vector>

namespace osrm::engine::routing_algorithms
{
namespace
{
// The table's counterpart of requiresForwardForce and requiresBackwardForce.
// Those compare rounded weights, which tie when two points sit within one
// weight unit, so compare positions on the geometry instead. Called only when
// both searches are still on their seeds at node, with zero graph weight.
inline bool isSourceDownstream(const NodeID node,
                               const PhantomNodeCandidates &sources,
                               const PhantomNodeCandidates &targets)
{
    // A candidate list holds at most one phantom per node.
    const auto source = std::find_if(
        sources.begin(),
        sources.end(),
        [&](const auto &phantom)
        {
            return (phantom.IsValidForwardSource() && phantom.forward_segment_id.id == node) ||
                   (phantom.IsValidReverseSource() && phantom.reverse_segment_id.id == node);
        });
    const auto target = std::find_if(
        targets.begin(),
        targets.end(),
        [&](const auto &phantom)
        {
            return (phantom.IsValidForwardTarget() && phantom.forward_segment_id.id == node) ||
                   (phantom.IsValidReverseTarget() && phantom.reverse_segment_id.id == node);
        });
    BOOST_ASSERT(source != sources.end());
    BOOST_ASSERT(target != targets.end());

    // Both phantoms lie on node, so they share its direction of travel.
    const bool forward = source->IsValidForwardSource() && source->forward_segment_id.id == node;
    const auto position = [forward](const PhantomNode &phantom)
    {
        return std::make_pair(forward ? phantom.forward_order_segment
                                      : phantom.reverse_order_segment,
                              phantom.order_ratio);
    };
    return forward ? position(*source) > position(*target) : position(*source) < position(*target);
}

struct NodeBucket
{
    NodeID middle_node;
    NodeID parent_node;
    unsigned column_index : 31; // a column in the weight/duration matrix
    unsigned from_clique_arc : 1;
    EdgeWeight weight;
    EdgeDuration duration;
    EdgeDistance distance;
    //! How much of @c weight is the walk into an open area rather than travel through the
    //! graph; see ManyToManyHeapData::approach, which this is copied from.
    EdgeWeight approach;

    NodeBucket(NodeID middle_node,
               NodeID parent_node,
               bool from_clique_arc,
               unsigned column_index,
               EdgeWeight weight,
               EdgeDuration duration,
               EdgeDistance distance,
               EdgeWeight approach = EdgeWeight{0})
        : middle_node(middle_node), parent_node(parent_node), column_index(column_index),
          from_clique_arc(from_clique_arc), weight(weight), duration(duration), distance(distance),
          approach(approach)
    {
    }

    NodeBucket(NodeID middle_node,
               NodeID parent_node,
               unsigned column_index,
               EdgeWeight weight,
               EdgeDuration duration,
               EdgeDistance distance,
               EdgeWeight approach = EdgeWeight{0})
        : middle_node(middle_node), parent_node(parent_node), column_index(column_index),
          from_clique_arc(false), weight(weight), duration(duration), distance(distance),
          approach(approach)
    {
    }

    // partial order comparison
    bool operator<(const NodeBucket &rhs) const
    { return std::tie(middle_node, column_index) < std::tie(rhs.middle_node, rhs.column_index); }

    // functor for equal_range
    struct Compare
    {
        bool operator()(const NodeBucket &lhs, const NodeID &rhs) const
        { return lhs.middle_node < rhs; }

        bool operator()(const NodeID &lhs, const NodeBucket &rhs) const
        { return lhs < rhs.middle_node; }
    };

    // functor for equal_range
    struct ColumnCompare
    {
        unsigned column_idx;

        ColumnCompare(unsigned column_idx) : column_idx(column_idx) {};

        bool operator()(const NodeBucket &lhs, const NodeID &rhs) const // lowerbound
        { return std::tie(lhs.middle_node, lhs.column_index) < std::tie(rhs, column_idx); }

        bool operator()(const NodeID &lhs, const NodeBucket &rhs) const // upperbound
        { return std::tie(lhs, column_idx) < std::tie(rhs.middle_node, rhs.column_index); }
    };
};
} // namespace

template <typename Algorithm>
std::pair<std::vector<EdgeDuration>, std::vector<EdgeDistance>>
manyToManySearch(SearchEngineData<Algorithm> &engine_working_data,
                 const DataFacade<Algorithm> &facade,
                 const std::vector<PhantomNodeCandidates> &candidates_list,
                 const std::vector<std::size_t> &source_indices,
                 const std::vector<std::size_t> &target_indices,
                 const bool calculate_distance);

} // namespace osrm::engine::routing_algorithms

#endif
