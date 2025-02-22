#include "extractor/turn_path_compressor.hpp"
#include "extractor/maneuver_override.hpp"
#include "extractor/restriction.hpp"

#include <algorithm>
#include <boost/assert.hpp>

namespace osrm::extractor
{

TurnPathCompressor::TurnPathCompressor(std::vector<TurnRestriction> &restrictions,
                                       std::vector<UnresolvedManeuverOverride> &maneuver_overrides)
{
    // Track all turn paths by their respective start/via/end nodes.
    auto index = [&](auto &element)
    {
        starts.insert({element.From(), &element});
        ends.insert({element.To(), &element});
        if (element.Type() == TurnPathType::VIA_WAY_TURN_PATH)
        {
            // Some of the via nodes can not be compressed so don't need tracking
            // (e.g. first and last via node of a restriction, instruction node of maneuver).
            // However, for the sake of simplicity we'll track them all.
            for (const auto &via_node : element.AsViaWayPath().via)
            {
                vias.insert({via_node, &element});
            }
        }
    };
    // We need to pass a reference to the index as we will mutate the turn path during compression.
    const auto index_starts_ends_vias = [&](auto &relation) { index(relation.turn_path); };

    std::for_each(restrictions.begin(), restrictions.end(), index_starts_ends_vias);
    std::for_each(maneuver_overrides.begin(), maneuver_overrides.end(), index_starts_ends_vias);
}

void TurnPathCompressor::Compress(const NodeID from, const NodeID via, const NodeID to)
{
    // handle turn restrictions
    // extract all start ptrs and move them from via to from.
    auto all_starts_range = starts.equal_range(via);
    std::vector<TurnPath *> start_ptrs;
    std::transform(all_starts_range.first,
                   all_starts_range.second,
                   std::back_inserter(start_ptrs),
                   [](const auto pair) { return pair.second; });

    const auto update_start = [&](auto ptr)
    {
        if (ptr->Type() == TurnPathType::VIA_NODE_TURN_PATH)
        {

            // ____ | from - p.from | via - p.via | to - p.to | ____
            auto &node_ptr = ptr->AsViaNodePath();
            BOOST_ASSERT(node_ptr.from == via);
            if (node_ptr.via == to)
            {
                node_ptr.from = from;
            }
            // ____ | to - p.from | via - p.via | from - p.to | ____
            else
            {
                BOOST_ASSERT(node_ptr.via == from);
                node_ptr.from = to;
            }
        }
        else
        {
            BOOST_ASSERT(ptr->Type() == TurnPathType::VIA_WAY_TURN_PATH);
            auto &way_ptr = ptr->AsViaWayPath();
            // ____ | from - p.from | via - p.via[0] | to - p[1..],p.to | ____
            BOOST_ASSERT(way_ptr.from == via);
            if (way_ptr.via.front() == to)
            {
                way_ptr.from = from;
            }
            // ____ | to - p.from | via - p.via[0] | from - p[1,..],p.to | ____
            else
            {
                BOOST_ASSERT(way_ptr.via.front() == from);
                way_ptr.from = to;
            }
        }
    };

    std::for_each(start_ptrs.begin(), start_ptrs.end(), update_start);

    // update the ptrs in our mapping
    starts.erase(via);

    const auto reinsert_start = [&](auto ptr) { starts.insert({ptr->From(), ptr}); };
    std::for_each(start_ptrs.begin(), start_ptrs.end(), reinsert_start);

    // extract all end ptrs and move them from via to to
    auto all_ends_range = ends.equal_range(via);
    std::vector<TurnPath *> end_ptrs;
    std::transform(all_ends_range.first,
                   all_ends_range.second,
                   std::back_inserter(end_ptrs),
                   [](const auto pair) { return pair.second; });

    const auto update_end = [&](auto ptr)
    {
        if (ptr->Type() == TurnPathType::VIA_NODE_TURN_PATH)
        {
            auto &node_ptr = ptr->AsViaNodePath();

            BOOST_ASSERT(node_ptr.to == via);
            // p.from | ____ - p.via | from - p.to | via - ____ | to
            if (node_ptr.via == from)
            {
                node_ptr.to = to;
            }
            // p.from | ____ - p.via | to - p.to | via - ____ | from
            else
            {
                BOOST_ASSERT(node_ptr.via == to);
                node_ptr.to = from;
            }
        }
        else
        {
            BOOST_ASSERT(ptr->Type() == TurnPathType::VIA_WAY_TURN_PATH);
            auto &way_ptr = ptr->AsViaWayPath();

            BOOST_ASSERT(way_ptr.to == via);
            // p.from,p.via[..,n-1] | ____ - p.via[n] | from - p.to | via - ____ | to
            if (way_ptr.via.back() == from)
            {
                way_ptr.to = to;
            }
            // p.from,p.via[..,n-1] | ____ - p.via[n] | to - p.to | via - ____ | from
            else
            {
                BOOST_ASSERT(way_ptr.via.back() == to);
                way_ptr.to = from;
            }
        }
    };
    std::for_each(end_ptrs.begin(), end_ptrs.end(), update_end);

    // update end ptrs in mapping
    ends.erase(via);

    const auto reinsert_end = [&](auto ptr) { ends.insert({ptr->To(), ptr}); };
    std::for_each(end_ptrs.begin(), end_ptrs.end(), reinsert_end);

    // remove compressed node from all via paths
    auto all_vias_range = vias.equal_range(via);

    const auto update_via = [&](auto restriction_pair)
    {
        BOOST_ASSERT(restriction_pair.second->Type() == TurnPathType::VIA_WAY_TURN_PATH);
        auto &way_ptr = restriction_pair.second->AsViaWayPath();
        BOOST_ASSERT(std::find(way_ptr.via.begin(), way_ptr.via.end(), via) != way_ptr.via.end());
        way_ptr.via.erase(std::remove(way_ptr.via.begin(), way_ptr.via.end(), via),
                          way_ptr.via.end());
    };
    std::for_each(all_vias_range.first, all_vias_range.second, update_via);

    // update via ptrs in mapping
    vias.erase(via);
}

} // namespace osrm::extractor
