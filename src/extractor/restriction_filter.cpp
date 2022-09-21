#include "extractor/restriction_filter.hpp"
#include "util/node_based_graph.hpp"
#include "util/timing_util.hpp"
#include "util/typedefs.hpp"

#include <algorithm>
#include <boost/assert.hpp>

namespace osrm
{
namespace extractor
{

std::vector<TurnRestriction>
removeInvalidRestrictions(std::vector<TurnRestriction> restrictions,
                          const util::NodeBasedDynamicGraph &node_based_graph)
{
    util::UnbufferedLog log;
    log << "Removing invalid restrictions...";
    TIMER_START(remove_invalid_restrictions);

    const auto is_valid_edge = [&node_based_graph](const auto from, const auto to) {
        const auto eid = node_based_graph.FindEdge(from, to);
        if (eid == SPECIAL_EDGEID)
        {
            util::Log(logDEBUG) << "Restriction has invalid edge: " << from << ", " << to;
            return false;
        }

        const auto &edge_data = node_based_graph.GetEdgeData(eid);
        if (edge_data.reversed)
        {
            util::Log(logDEBUG) << "Restriction has non-traversable edge: " << from << ", " << to;
            return false;
        }
        return true;
    };

    const auto is_valid_node = [is_valid_edge](const auto &node_restriction) {
        return is_valid_edge(node_restriction.from, node_restriction.via) &&
               is_valid_edge(node_restriction.via, node_restriction.to);
    };

    const auto is_valid_way = [is_valid_edge](const auto &way_restriction) {
        if (!is_valid_edge(way_restriction.from, way_restriction.via.front()))
            return false;

        const auto invalid_it = std::adjacent_find(
            way_restriction.via.begin(),
            way_restriction.via.end(),
            [&](auto via_from, auto via_to) { return !is_valid_edge(via_from, via_to); });
        if (invalid_it != way_restriction.via.end())
            return false;

        return is_valid_edge(way_restriction.via.back(), way_restriction.to);
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
    const auto num_removed = std::distance(end_valid_restrictions, restrictions.end());
    restrictions.erase(end_valid_restrictions, restrictions.end());

    TIMER_STOP(remove_invalid_restrictions);
    log << "removed " << num_removed << " invalid restrictions, after "
        << TIMER_SEC(remove_invalid_restrictions) << "s";

    return restrictions;
}

} // namespace extractor
} // namespace osrm
