#include "extractor/way_restriction_map.hpp"

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

} // namespace

WayRestrictionMap::WayRestrictionMap(const std::vector<TurnRestriction> &turn_restrictions)
{
    // get all way restrictions
    const auto extract_restrictions = [this](const auto &turn_restriction) {
        if (turn_restriction.Type() == RestrictionType::WAY_RESTRICTION)
        {
            const auto &way = turn_restriction.AsWayRestriction();

            // so far we can only handle restrictions that are not interrupted
            if (way.in_restriction.via == way.out_restriction.from &&
                way.in_restriction.to == way.out_restriction.via)
                restriction_data.push_back(turn_restriction);
        }
    };
    std::for_each(turn_restrictions.begin(), turn_restrictions.end(), extract_restrictions);

    const auto as_duplicated_node =
        [](auto const &restriction) -> std::tuple<NodeID, NodeID, NodeID> {
        auto &way = restriction.AsWayRestriction();
        // group restrictions by the via-way. On same via-ways group by from
        return std::make_tuple(
            way.in_restriction.via, way.out_restriction.via, way.in_restriction.from);
    };

    const auto by_duplicated_node = [&](auto const &lhs, auto const &rhs) {
        return as_duplicated_node(lhs) < as_duplicated_node(rhs);
    };

    std::sort(restriction_data.begin(), restriction_data.end(), by_duplicated_node);

    std::size_t index = 0, duplication_id = 0;
    // map all way restrictions into access containers
    const auto prepare_way_restriction = [this, &index, &duplication_id, as_duplicated_node](
        const auto &restriction) {
        const auto &way = restriction.AsWayRestriction();
        restriction_starts.insert(
            std::make_pair(std::make_pair(way.in_restriction.from, way.in_restriction.via), index));
        ++index;
    };
    std::for_each(restriction_data.begin(), restriction_data.end(), prepare_way_restriction);

    std::size_t offset = 1;
    // the first group starts at 0
    if (!restriction_data.empty())
        duplicated_node_groups.push_back(0);

    auto const add_offset_on_new_groups = [&](auto const &lhs, auto const &rhs) {
        BOOST_ASSERT(rhs == restriction_data[offset]);
        // add a new lower bound for rhs
        if (as_duplicated_node(lhs) != as_duplicated_node(rhs))
            duplicated_node_groups.push_back(offset);
        ++offset;
        return false; // continue until the end
    };
    std::adjacent_find(restriction_data.begin(), restriction_data.end(), add_offset_on_new_groups);
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

std::size_t WayRestrictionMap::AsDuplicatedNodeID(const std::size_t restriction_id) const
{
    return std::distance(duplicated_node_groups.begin(),
                         std::upper_bound(duplicated_node_groups.begin(),
                                          duplicated_node_groups.end(),
                                          restriction_id)) -
           1;
}

util::range<std::size_t> WayRestrictionMap::DuplicatedNodeIDs(const NodeID from,
                                                              const NodeID to) const
{
    const auto duplicated_node_range_itr = std::equal_range(
        restriction_data.begin(), restriction_data.end(), std::make_tuple(from, to), FindViaWay());

    const auto as_restriction_id = [this](const auto itr) {
        return std::distance(restriction_data.begin(), itr);
    };

    return util::irange<std::size_t>(
        AsDuplicatedNodeID(as_restriction_id(duplicated_node_range_itr.first)),
        AsDuplicatedNodeID(as_restriction_id(duplicated_node_range_itr.second)));
}

bool WayRestrictionMap::IsRestricted(std::size_t duplicated_node, const NodeID to) const
{
    // loop over all restrictions associated with the node. Mark as restricted based on
    // is_only/restricted targets
    for (std::size_t restriction_index = duplicated_node_groups[duplicated_node];
         restriction_index != duplicated_node_groups[duplicated_node + 1];
         ++restriction_index)
    {
        const auto &restriction = restriction_data[restriction_index];
        const auto &way = restriction.AsWayRestriction();

        if (restriction.is_only)
            return way.out_restriction.to != to;
        else if (to == way.out_restriction.to)
            return true;
    }
    return false;
}

TurnRestriction const &WayRestrictionMap::GetRestriction(const std::size_t id) const
{
    return restriction_data[id];
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
                       return {representative_id, way.in_restriction.via, way.out_restriction.via};
                   });
    return result;
}

NodeID WayRestrictionMap::RemapIfRestricted(const NodeID edge_based_node,
                                            const NodeID node_based_from,
                                            const NodeID node_based_via,
                                            const NodeID node_based_to,
                                            const NodeID number_of_edge_based_nodes) const
{
    auto range = restriction_starts.equal_range(std::make_pair(node_based_from, node_based_via));

    // returns true if the ID saved in an iterator belongs to a turn restriction that references
    // node_based_to as destination of the `in_restriction`
    const auto restriction_targets_to = [node_based_to, this](const auto &pair) {
        return restriction_data[pair.second].AsWayRestriction().in_restriction.to == node_based_to;
    };
    const auto itr = std::find_if(range.first, range.second, restriction_targets_to);

    // in case we found a matching restriction, we can remap the edge_based_node
    if (itr != range.second)
        return number_of_edge_based_nodes - NumberOfDuplicatedNodes() +
               AsDuplicatedNodeID(itr->second);
    else
        return edge_based_node;
}

} // namespace extractor
} // namespace osrm
