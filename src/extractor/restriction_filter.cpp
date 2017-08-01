#include "extractor/restriction_filter.hpp"
#include "util/node_based_graph.hpp"
#include "util/typedefs.hpp"

#include <algorithm>
#include <boost/assert.hpp>

namespace osrm
{
namespace extractor
{

std::vector<ConditionalTurnRestriction>
removeInvalidRestrictions(std::vector<ConditionalTurnRestriction> restrictions,
                          const util::NodeBasedDynamicGraph &node_based_graph)
{
    // definition of what we presume to be a valid via-node restriction
    const auto is_valid_node = [&node_based_graph](const auto &node_restriction) {
        // a valid restriction needs to be connected to both its from and to locations
        bool found_from = false, found_to = false;
        for (auto eid : node_based_graph.GetAdjacentEdgeRange(node_restriction.via))
        {
            const auto target = node_based_graph.GetTarget(eid);
            if (target == node_restriction.from)
                found_from = true;
            if (target == node_restriction.to)
                found_to = true;
        }

        if (!found_from || !found_to)
            return false;

        return true;
    };

    // definition of what we presume to be a valid via-way restriction
    const auto is_valid_way = [&node_based_graph, is_valid_node](const auto &way_restriction) {
        const auto eid = node_based_graph.FindEdge(way_restriction.in_restriction.via,
                                                   way_restriction.out_restriction.via);

        // ability filter, we currently cannot handle restrictions that do not match up in geometry:
        // restrictions cannot be interrupted by traffic signals or other similar entities that
        // cause node penalties
        if ((way_restriction.in_restriction.via != way_restriction.out_restriction.from) ||
            (way_restriction.out_restriction.via != way_restriction.in_restriction.to))
            return false;

        // the edge needs to exit (we cannot handle intermediate stuff, so far)
        if (eid == SPECIAL_EDGEID)
            return false;

        const auto &data = node_based_graph.GetEdgeData(eid);

        // edge needs to be traversable for a valid restrction
        if (data.reversed)
            return false;

        // is the in restriction referencing the correct nodes
        if (!is_valid_node(way_restriction.in_restriction))
            return false;

        // is the out restriction referencing the correct nodes
        if (!is_valid_node(way_restriction.out_restriction))
            return false;

        return true;
    };

    const auto is_invalid = [is_valid_way, is_valid_node](const auto &restriction) {
        if (restriction.Type() == RestrictionType::NODE_RESTRICTION)
        {
            return !is_valid_node(restriction.AsNodeRestriction());
        }
        else
        {
            BOOST_ASSERT(restriction.Type() == RestrictionType::WAY_RESTRICTION);
            return !is_valid_way(restriction.AsWayRestriction());
        }
    };

    const auto end_valid_restrictions =
        std::remove_if(restrictions.begin(), restrictions.end(), is_invalid);
    restrictions.erase(end_valid_restrictions, restrictions.end());

    return restrictions;
}

} // namespace extractor
} // namespace osrm
