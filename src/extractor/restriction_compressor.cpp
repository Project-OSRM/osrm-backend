#include "extractor/restriction_compressor.hpp"
#include "extractor/restriction.hpp"

#include <algorithm>
#include <boost/assert.hpp>

namespace osrm
{
namespace extractor
{

RestrictionCompressor::RestrictionCompressor(
    std::vector<TurnRestriction> &restrictions,
    std::vector<UnresolvedManeuverOverride> &maneuver_overrides)
{
    // add a turn restriction ptr to the starts/ends maps, needs to be a reference!
    auto index = [&](auto &element) {
        starts.insert({element.From(), &element});
        ends.insert({element.To(), &element});
        if (element.Type() == RestrictionType::WAY_RESTRICTION)
        {
            const auto &way_via = element.AsWayRestriction().via;
            BOOST_ASSERT(way_via.size() >= 2);
            // No need to track the first and last via nodes as they will not be compressed.
            for (const auto &via_node :
                 boost::make_iterator_range(way_via.begin() + 1, way_via.end() - 1))
            {
                vias.insert({via_node, &element});
            }
        }
    };
    // !needs to be reference, so we can get the correct address
    const auto index_starts_ends_vias = [&](auto &restriction) { index(restriction); };

    // add all restrictions as their respective start/via/end pointers
    std::for_each(restrictions.begin(), restrictions.end(), index_starts_ends_vias);

    auto index_maneuver = [&](auto &maneuver) {
        for (auto &turn : maneuver.turn_sequence)
        {
            maneuver_starts.insert({turn.from, &turn});
            maneuver_ends.insert({turn.to, &turn});
        }
    };
    // !needs to be reference, so we can get the correct address
    std::for_each(maneuver_overrides.begin(), maneuver_overrides.end(), [&](auto &maneuver) {
        index_maneuver(maneuver);
    });
}

void RestrictionCompressor::Compress(const NodeID from, const NodeID via, const NodeID to)
{
    // handle turn restrictions
    // extract all start ptrs and move them from via to from.
    auto all_starts_range = starts.equal_range(via);
    std::vector<TurnRestriction *> start_ptrs;
    std::transform(all_starts_range.first,
                   all_starts_range.second,
                   std::back_inserter(start_ptrs),
                   [](const auto pair) { return pair.second; });

    const auto update_start = [&](auto ptr) {
        if (ptr->Type() == RestrictionType::NODE_RESTRICTION)
        {

            // ____ | from - p.from | via - p.via | to - p.to | ____
            auto &node_ptr = ptr->AsNodeRestriction();
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
            BOOST_ASSERT(ptr->Type() == RestrictionType::WAY_RESTRICTION);
            auto &way_ptr = ptr->AsWayRestriction();
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
    std::vector<TurnRestriction *> end_ptrs;
    std::transform(all_ends_range.first,
                   all_ends_range.second,
                   std::back_inserter(end_ptrs),
                   [](const auto pair) { return pair.second; });

    const auto update_end = [&](auto ptr) {
        if (ptr->Type() == RestrictionType::NODE_RESTRICTION)
        {
            auto &node_ptr = ptr->AsNodeRestriction();

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
            BOOST_ASSERT(ptr->Type() == RestrictionType::WAY_RESTRICTION);
            auto &way_ptr = ptr->AsWayRestriction();

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

    const auto update_via = [&](auto restriction_pair) {
        BOOST_ASSERT(restriction_pair.second->Type() == RestrictionType::WAY_RESTRICTION);
        auto &way_ptr = restriction_pair.second->AsWayRestriction();
        BOOST_ASSERT(std::find(way_ptr.via.begin(), way_ptr.via.end(), via) != way_ptr.via.end());
        way_ptr.via.erase(std::remove(way_ptr.via.begin(), way_ptr.via.end(), via),
                          way_ptr.via.end());
    };
    std::for_each(all_vias_range.first, all_vias_range.second, update_via);

    // update via ptrs in mapping
    vias.erase(via);

    /**********************************************************************************************/

    // handle maneuver overrides from nodes
    // extract all startptrs
    auto maneuver_starts_range = maneuver_starts.equal_range(via);
    std::vector<NodeBasedTurn *> mnv_start_ptrs;
    std::transform(maneuver_starts_range.first,
                   maneuver_starts_range.second,
                   std::back_inserter(mnv_start_ptrs),
                   [](const auto pair) { return pair.second; });

    // update from nodes of maneuver overrides
    const auto update_start_mnv = [&](auto ptr) {
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
    std::for_each(mnv_start_ptrs.begin(), mnv_start_ptrs.end(), update_start_mnv);

    // update the ptrs in our mapping
    maneuver_starts.erase(via);
    const auto reinsert_start_mnv = [&](auto ptr) { maneuver_starts.insert({ptr->from, ptr}); };
    std::for_each(mnv_start_ptrs.begin(), mnv_start_ptrs.end(), reinsert_start_mnv);

    /**********************************************************************************************/
    // handle maneuver override to nodes
    // extract all end ptrs and move them from via to to
    auto maneuver_ends_range = maneuver_ends.equal_range(via);
    std::vector<NodeBasedTurn *> mnv_end_ptrs;
    std::transform(maneuver_ends_range.first,
                   maneuver_ends_range.second,
                   std::back_inserter(mnv_end_ptrs),
                   [](const auto pair) { return pair.second; });

    const auto update_end_mnv = [&](auto ptr) {
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
    std::for_each(mnv_end_ptrs.begin(), mnv_end_ptrs.end(), update_end_mnv);

    // update end ptrs in mapping
    maneuver_ends.erase(via);

    const auto reinsert_end_mnvs = [&](auto ptr) { maneuver_ends.insert({ptr->to, ptr}); };
    std::for_each(mnv_end_ptrs.begin(), mnv_end_ptrs.end(), reinsert_end_mnvs);
}

} // namespace extractor
} // namespace osrm
