#include "extractor/turn_path_filter.hpp"
#include "extractor/maneuver_override.hpp"
#include "util/node_based_graph.hpp"
#include "util/timing_util.hpp"
#include "util/typedefs.hpp"

#include <algorithm>
#include <boost/assert.hpp>

namespace osrm::extractor
{

template <typename T>
std::vector<T> removeInvalidTurnPaths(std::vector<T> turn_relations,
                                      const util::NodeBasedDynamicGraph &node_based_graph)
{
    util::UnbufferedLog log;
    log << "Removing invalid " << T::Name() << "s...";
    TIMER_START(remove_invalid_turn_paths);

    const auto is_valid_edge = [&node_based_graph](const auto from, const auto to)
    {
        const auto eid = node_based_graph.FindEdge(from, to);
        if (eid == SPECIAL_EDGEID)
        {
            util::Log(logDEBUG) << T::Name() << " has invalid edge: " << from << ", " << to;
            return false;
        }

        const auto &edge_data = node_based_graph.GetEdgeData(eid);
        if (edge_data.reversed)
        {
            util::Log(logDEBUG) << T::Name() << " has non-traversable edge: " << from << ", " << to;
            return false;
        }
        return true;
    };

    const auto is_valid_node = [is_valid_edge](const auto &via_node_path)
    {
        return is_valid_edge(via_node_path.from, via_node_path.via) &&
               is_valid_edge(via_node_path.via, via_node_path.to);
    };

    const auto is_valid_way = [is_valid_edge](const auto &via_way_path)
    {
        if (!is_valid_edge(via_way_path.from, via_way_path.via.front()))
            return false;

        const auto invalid_it = std::adjacent_find(via_way_path.via.begin(),
                                                   via_way_path.via.end(),
                                                   [&](auto via_from, auto via_to)
                                                   { return !is_valid_edge(via_from, via_to); });
        if (invalid_it != via_way_path.via.end())
            return false;

        return is_valid_edge(via_way_path.via.back(), via_way_path.to);
    };

    const auto is_invalid = [is_valid_way, is_valid_node](const auto &turn_relation)
    {
        if (turn_relation.turn_path.Type() == TurnPathType::VIA_NODE_TURN_PATH)
        {
            return !is_valid_node(turn_relation.turn_path.AsViaNodePath());
        }

        BOOST_ASSERT(turn_relation.turn_path.Type() == TurnPathType::VIA_WAY_TURN_PATH);
        return !is_valid_way(turn_relation.turn_path.AsViaWayPath());
    };

    const auto end_valid_relations =
        std::remove_if(turn_relations.begin(), turn_relations.end(), is_invalid);
    const auto num_removed = std::distance(end_valid_relations, turn_relations.end());
    turn_relations.erase(end_valid_relations, turn_relations.end());

    TIMER_STOP(remove_invalid_turn_paths);
    log << "removed " << num_removed << " invalid " << T::Name() << "s, after "
        << TIMER_SEC(remove_invalid_turn_paths) << "s";

    return turn_relations;
}

template std::vector<TurnRestriction> removeInvalidTurnPaths<>(std::vector<TurnRestriction>,
                                                               const util::NodeBasedDynamicGraph &);
template std::vector<UnresolvedManeuverOverride>
removeInvalidTurnPaths<>(std::vector<UnresolvedManeuverOverride>,
                         const util::NodeBasedDynamicGraph &);

} // namespace osrm::extractor
