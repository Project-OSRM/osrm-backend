#include "extractor/restriction_compressor.hpp"
#include "extractor/restriction.hpp"

#include <algorithm>
#include <boost/assert.hpp>
#include <utility>

namespace osrm
{
namespace extractor
{

RestrictionCompressor::RestrictionCompressor(
    std::vector<TurnRestriction> &restrictions,
    std::vector<ConditionalTurnRestriction> &conditional_turn_restrictions)
{
    // add a node restriction ptr to the starts/ends maps, needs to be a reference!
    auto index = [&](auto &element) {
        starts.insert(std::make_pair(element.from, &element));
        ends.insert(std::make_pair(element.to, &element));
    };
    // !needs to be reference, so we can get the correct address
    const auto index_starts_and_ends = [&](auto &restriction) {
        if (restriction.Type() == RestrictionType::WAY_RESTRICTION)
        {
            auto &way_restriction = restriction.AsWayRestriction();
            index(way_restriction.in_restriction);
            index(way_restriction.out_restriction);
        }
        else
        {
            BOOST_ASSERT(restriction.Type() == RestrictionType::NODE_RESTRICTION);
            auto &node_restriction = restriction.AsNodeRestriction();
            index(node_restriction);
        }
    };

    // add all restrictions as their respective startend pointers
    std::for_each(restrictions.begin(), restrictions.end(), index_starts_and_ends);
    std::for_each(conditional_turn_restrictions.begin(),
                  conditional_turn_restrictions.end(),
                  index_starts_and_ends);
}

void RestrictionCompressor::Compress(const NodeID from, const NodeID via, const NodeID to)
{
    // extract all startptrs and move them from via to from.
    auto all_starts_range = starts.equal_range(via);
    std::vector<NodeRestriction *> start_ptrs;
    std::transform(all_starts_range.first,
                   all_starts_range.second,
                   std::back_inserter(start_ptrs),
                   [](const auto pair) { return pair.second; });

    const auto update_start = [&](auto ptr) {
        // ____ | from - p.from | via - p.via | to - p.to | ____
        BOOST_ASSERT(ptr->from == via);
        if (ptr->via == to)
        {
            ptr->from = from;
        }
        // ____ | to - p.from | via - p.via | from - p.to | ____
        else
        {
            BOOST_ASSERT(ptr->via == from);
            ptr->from = to;
        }
    };

    std::for_each(start_ptrs.begin(), start_ptrs.end(), update_start);

    // update the ptrs in our mapping
    starts.erase(via);

    const auto reinsert_start = [&](auto ptr) { starts.insert(std::make_pair(ptr->from, ptr)); };
    std::for_each(start_ptrs.begin(), start_ptrs.end(), reinsert_start);

    // extract all end ptrs and move them from via to to
    auto all_ends_range = ends.equal_range(via);
    std::vector<NodeRestriction *> end_ptrs;
    std::transform(all_ends_range.first,
                   all_ends_range.second,
                   std::back_inserter(end_ptrs),
                   [](const auto pair) { return pair.second; });

    const auto update_end = [&](auto ptr) {
        BOOST_ASSERT(ptr->to == via);
        // p.from | ____ - p.via | from - p.to | via - ____ | to
        if (ptr->via == from)
        {
            ptr->to = to;
        }
        // p.from | ____ - p.via | to - p.to | via - ____ | from
        else
        {
            BOOST_ASSERT(ptr->via == to);
            ptr->to = from;
        }
    };
    std::for_each(end_ptrs.begin(), end_ptrs.end(), update_end);

    // update end ptrs in mapping
    ends.erase(via);

    const auto reinsert_end = [&](auto ptr) { ends.insert(std::make_pair(ptr->to, ptr)); };
    std::for_each(end_ptrs.begin(), end_ptrs.end(), reinsert_end);
}

} // namespace extractor
} // namespace osrm
