#include "extractor/way_restriction_map.hpp"
#include "util/for_each_pair.hpp"

#include <functional>
#include <iterator>
#include <tuple>
#include <utility>

namespace osrm
{
namespace extractor
{

namespace
{
struct FindViaWay
{
    bool operator()(const std::tuple<NodeID, NodeID> value,
                    const TurnRestriction &restriction) const
    {
        const auto &way = restriction.AsWayRestriction();
        return value < std::tie(way.in_restriction.via, way.out_restriction.via);
    }
    bool operator()(const TurnRestriction &restriction,
                    const std::tuple<NodeID, NodeID> value) const
    {
        const auto &way = restriction.AsWayRestriction();
        return std::tie(way.in_restriction.via, way.out_restriction.via) < value;
    }
};

template <typename restriction_type> auto asDuplicatedNode(const restriction_type &restriction)
{
    auto &way = restriction.AsWayRestriction();
    // group restrictions by the via-way. On same via-ways group by from
    return std::tie(way.in_restriction.via, way.out_restriction.via, way.in_restriction.from);
}

template <typename restriction_type> struct CompareByDuplicatedNode
{
    bool operator()(const ConditionalTurnRestriction &lhs, const ConditionalTurnRestriction &rhs)
    {
        if (asDuplicatedNode(lhs) < asDuplicatedNode(rhs))
        {
            return true;
        }
        else if (asDuplicatedNode(rhs) < asDuplicatedNode(lhs))
        {
            return false;
        }
        else
        {
            const auto lhs_to = lhs.AsWayRestriction().out_restriction.to;
            const auto rhs_to = rhs.AsWayRestriction().out_restriction.to;

            const bool has_conditions_lhs = !lhs.condition.empty();
            const bool has_conditions_rhs = !rhs.condition.empty();

            return std::tie(lhs_to, lhs.is_only, has_conditions_lhs) <
                   std::tie(rhs_to, rhs.is_only, has_conditions_rhs);
        }
    }
};

template <typename restriction_type>
std::vector<restriction_type>
extractRestrictions(const std::vector<restriction_type> &turn_restrictions)
{
    std::vector<restriction_type> result;
    for (const auto &turn_restriction : turn_restrictions)
    {
        if (turn_restriction.Type() == RestrictionType::WAY_RESTRICTION)
        {
            const auto &way = turn_restriction.AsWayRestriction();

            // so far we can only handle restrictions that are not interrupted
            if (way.in_restriction.via == way.out_restriction.from &&
                way.in_restriction.to == way.out_restriction.via)
                result.push_back(turn_restriction);
        }
    }
    std::sort(result.begin(), result.end(), CompareByDuplicatedNode<restriction_type>());
    auto new_end = std::unique(result.begin(), result.end());
    result.erase(new_end, result.end());
    return result;
}

template <typename restriction_type> struct ByInFromAndVia
{
    std::pair<NodeID, NodeID> operator()(const restriction_type &restriction)
    {
        const auto &way = restriction.AsWayRestriction();
        return std::make_pair(way.in_restriction.from, way.in_restriction.via);
    }
};

} // namespace

// get all way restrictions
WayRestrictionMap::WayRestrictionMap(
    const std::vector<ConditionalTurnRestriction> &turn_restrictions_)
    : restriction_data(extractRestrictions(turn_restrictions_)),
      restriction_starts(restriction_data, ByInFromAndVia<ConditionalTurnRestriction>())
{
    std::size_t offset = 1;
    // the first group starts at 0
    if (!restriction_data.empty())
        duplicated_node_groups.push_back(0);

    auto const add_offset_on_new_groups = [&](auto const &lhs, auto const &rhs) {
        BOOST_ASSERT(rhs == restriction_data[offset]);
        BOOST_ASSERT(lhs.Type() == RestrictionType::WAY_RESTRICTION);
        BOOST_ASSERT(rhs.Type() == RestrictionType::WAY_RESTRICTION);
        // add a new lower bound for rhs
        if (asDuplicatedNode(lhs) != asDuplicatedNode(rhs))
            duplicated_node_groups.push_back(offset);
        ++offset;
    };
    util::for_each_pair(restriction_data.begin(), restriction_data.end(), add_offset_on_new_groups);
    duplicated_node_groups.push_back(restriction_data.size());
}

std::size_t WayRestrictionMap::NumberOfDuplicatedNodes() const
{
    return duplicated_node_groups.size() - 1;
}

bool WayRestrictionMap::IsViaWay(const NodeID from, const NodeID to) const
{
    // safe-guards
    if (restriction_data.empty())
        return false;

    const auto itr = std::lower_bound(
        restriction_data.begin(), restriction_data.end(), std::make_tuple(from, to), FindViaWay());

    // no fitting restriction
    if (itr == restriction_data.end())
        return false;

    const auto &way = itr->AsWayRestriction();

    return way.out_restriction.from == from && way.out_restriction.via == to;
}

DuplicatedNodeID WayRestrictionMap::AsDuplicatedNodeID(const RestrictionID restriction_id) const
{
    const auto upper_bound_restriction = std::upper_bound(
        duplicated_node_groups.begin(), duplicated_node_groups.end(), restriction_id);
    const auto distance_to_upper_bound =
        std::distance(duplicated_node_groups.begin(), upper_bound_restriction);
    return distance_to_upper_bound - 1;
}

std::vector<DuplicatedNodeID> WayRestrictionMap::DuplicatedNodeIDs(const NodeID from,
                                                                   const NodeID to) const
{
    const auto duplicated_node_range_itr = std::equal_range(
        restriction_data.begin(), restriction_data.end(), std::make_tuple(from, to), FindViaWay());

    const auto as_restriction_id = [this](const auto itr) {
        return std::distance(restriction_data.begin(), itr);
    };

    auto first = AsDuplicatedNodeID(as_restriction_id(duplicated_node_range_itr.first));
    auto end = AsDuplicatedNodeID(as_restriction_id(duplicated_node_range_itr.second));

    std::vector<DuplicatedNodeID> result(end - first);
    std::iota(result.begin(), result.end(), first);

    return result;
}

bool WayRestrictionMap::IsRestricted(DuplicatedNodeID duplicated_node, const NodeID to) const
{
    // loop over all restrictions associated with the node. Mark as restricted based on
    // is_only/restricted targets
    for (RestrictionID restriction_index = duplicated_node_groups[duplicated_node];
         restriction_index != duplicated_node_groups[duplicated_node + 1];
         ++restriction_index)
    {
        BOOST_ASSERT(restriction_index < restriction_data.size());
        const auto &restriction = restriction_data[restriction_index];
        const auto &way = restriction.AsWayRestriction();

        if (restriction.is_only)
            return way.out_restriction.to != to;
        else if (to == way.out_restriction.to)
            return true;
    }
    return false;
}

ConditionalTurnRestriction const &
WayRestrictionMap::GetRestriction(DuplicatedNodeID duplicated_node, const NodeID to) const
{
    // loop over all restrictions associated with the node. Mark as restricted based on
    // is_only/restricted targets
    for (RestrictionID restriction_index = duplicated_node_groups[duplicated_node];
         restriction_index != duplicated_node_groups[duplicated_node + 1];
         ++restriction_index)
    {
        BOOST_ASSERT(restriction_index < restriction_data.size());
        const auto &restriction = restriction_data[restriction_index];
        const auto &way = restriction.AsWayRestriction();

        if (restriction.is_only && (way.out_restriction.to != to))
        {
            return restriction;
        }
        else if (!restriction.is_only && (to == way.out_restriction.to))
        {
            return restriction;
        }
    }
    throw("Asking for the restriction of an unrestricted turn. Check with `IsRestricted` before "
          "calling GetRestriction");
}

std::vector<WayRestrictionMap::ViaWay> WayRestrictionMap::DuplicatedNodeRepresentatives() const
{
    std::vector<ViaWay> result;
    result.reserve(NumberOfDuplicatedNodes());
    std::transform(duplicated_node_groups.begin(),
                   duplicated_node_groups.end() - 1,
                   std::back_inserter(result),
                   [&](auto const representative_id) -> ViaWay {
                       auto &way = restriction_data[representative_id].AsWayRestriction();
                       return {way.in_restriction.via, way.out_restriction.via};
                   });
    return result;
}

NodeID WayRestrictionMap::RemapIfRestricted(const NodeID edge_based_node,
                                            const NodeID node_based_from,
                                            const NodeID node_based_via,
                                            const NodeID node_based_to,
                                            const NodeID number_of_edge_based_nodes) const
{
    auto range = restriction_starts.Restrictions(node_based_from, node_based_via);

    // returns true if the ID saved in an iterator belongs to a turn restriction that references
    // node_based_to as destination of the `in_restriction`
    const auto restriction_targets_to = [node_based_to](const auto &pair) {
        return pair.second->AsWayRestriction().in_restriction.to == node_based_to;
    };
    const auto itr = std::find_if(range.first, range.second, restriction_targets_to);

    // in case we found a matching restriction, we can remap the edge_based_node
    if (itr != range.second)
        return number_of_edge_based_nodes - NumberOfDuplicatedNodes() +
               AsDuplicatedNodeID(itr->second - &restriction_data[0]);
    else
        return edge_based_node;
}

} // namespace extractor
} // namespace osrm
