#ifndef OSRM_EXTRACTOR_RESTRICTION_INDEX_HPP_
#define OSRM_EXTRACTOR_RESTRICTION_INDEX_HPP_

#include "extractor/restriction.hpp"
#include "util/typedefs.hpp"

#include <boost/unordered_map.hpp>

#include <utility>
#include <vector>

namespace osrm
{
namespace extractor
{

// allows easy check for whether a node intersection is present at a given intersection
template <typename restriction_type> class RestrictionIndex
{
  public:
    using value_type = restriction_type;

    template <typename extractor_type>
    RestrictionIndex(std::vector<restriction_type> &restrictions, extractor_type extractor);

    bool IsIndexed(NodeID first, NodeID second) const;

    auto Restrictions(NodeID first, NodeID second) const
    {
        return restriction_hash.equal_range(std::make_pair(first, second));
    };

    auto Size() const { return restriction_hash.size(); }

  private:
    boost::unordered_multimap<std::pair<NodeID, NodeID>, restriction_type *> restriction_hash;
};

template <typename restriction_type>
template <typename extractor_type>
RestrictionIndex<restriction_type>::RestrictionIndex(std::vector<restriction_type> &restrictions,
                                                     extractor_type extractor)
{
    // build a multi-map
    for (auto &restriction : restrictions)
        restriction_hash.insert(std::make_pair(extractor(restriction), &restriction));
}

template <typename restriction_type>
bool RestrictionIndex<restriction_type>::IsIndexed(const NodeID first, const NodeID second) const
{
    return restriction_hash.count(std::make_pair(first, second));
}

struct IndexNodeByFromAndVia
{
    std::pair<NodeID, NodeID> operator()(const TurnRestriction &restriction)
    {
        const auto &node = restriction.AsNodeRestriction();
        return std::make_pair(node.from, node.via);
    };
};

// check wheter a turn is restricted within a restriction_index
template <typename restriction_map_type>
std::pair<bool, typename restriction_map_type::value_type *>
isRestricted(const NodeID from,
             const NodeID via,
             const NodeID to,
             const restriction_map_type &restriction_map)
{
    const auto range = restriction_map.Restrictions(from, via);

    // check if a given node_restriction is targeting node
    const auto to_is_restricted = [to](const auto &pair) {
        const auto &restriction = *pair.second;
        if (restriction.Type() == RestrictionType::NODE_RESTRICTION)
        {
            auto const &as_node = restriction.AsNodeRestriction();
            auto const restricted = restriction.is_only ? (to != as_node.to) : (to == as_node.to);

            return restricted;
        }
        return false;
    };

    auto itr = std::find_if(range.first, range.second, to_is_restricted);

    if (itr != range.second)
        return {true, itr->second};
    else
        return {false, NULL};
}

using RestrictionMap = RestrictionIndex<TurnRestriction>;
using ConditionalRestrictionMap = RestrictionIndex<ConditionalTurnRestriction>;

} // namespace extractor
} // namespace osrm

#endif // OSRM_EXTRACTOR_RESTRICTION_INDEX_HPP_
