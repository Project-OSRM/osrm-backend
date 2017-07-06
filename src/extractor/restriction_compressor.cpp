#include "extractor/restriction_compressor.hpp"
#include "extractor/restriction.hpp"

#include <algorithm>
#include <boost/assert.hpp>
#include <utility>

namespace osrm
{
namespace extractor
{

RestrictionCompressor::RestrictionCompressor(std::vector<TurnRestriction> &restrictions)
{
    // add a node restriction ptr to the heads/tails maps, needs to be a reference!
    auto index = [&](auto &element) {
        heads.insert(std::make_pair(element.from, &element));
        tails.insert(std::make_pair(element.to, &element));
    };
    // !needs to be reference, so we can get the correct address
    const auto index_heads_and_tails = [&](auto &restriction) {
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

    // add all restrictions as their respective head-tail pointers
    std::for_each(restrictions.begin(), restrictions.end(), index_heads_and_tails);
}

void RestrictionCompressor::Compress(const NodeID from, const NodeID via, const NodeID to)
{
    const auto get_value = [](const auto pair) { return pair.second; };

    // extract all head ptrs and move them from via to from.
    auto all_heads_range = heads.equal_range(via);
    std::vector<NodeRestriction *> head_ptrs;
    std::transform(
        all_heads_range.first, all_heads_range.second, std::back_inserter(head_ptrs), get_value);

    const auto update_head = [&](auto ptr) {
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

    std::for_each(head_ptrs.begin(), head_ptrs.end(), update_head);

    const auto reinsert_head = [&](auto ptr) { heads.insert(std::make_pair(ptr->from, ptr)); };

    // update the ptrs in our mapping
    heads.erase(via);
    std::for_each(head_ptrs.begin(), head_ptrs.end(), reinsert_head);

    // extract all tail ptrs and move them from via to to
    auto all_tails_range = tails.equal_range(via);
    std::vector<NodeRestriction *> tail_ptrs;
    std::transform(
        all_tails_range.first, all_tails_range.second, std::back_inserter(tail_ptrs), get_value);

    const auto update_tail = [&](auto ptr) {
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

    const auto reinsert_tail = [&](auto ptr) { tails.insert(std::make_pair(ptr->to, ptr)); };

    std::for_each(tail_ptrs.begin(), tail_ptrs.end(), update_tail);

    // update tail ptrs in mapping
    tails.erase(via);
    std::for_each(tail_ptrs.begin(), tail_ptrs.end(), reinsert_tail);
}

} // namespace extractor
} // namespace osrm
