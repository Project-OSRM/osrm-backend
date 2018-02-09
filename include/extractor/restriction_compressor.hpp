#ifndef OSRM_EXTRACTOR_RESTRICTION_COMPRESSOR_HPP_
#define OSRM_EXTRACTOR_RESTRICTION_COMPRESSOR_HPP_

#include "extractor/maneuver_override.hpp"
#include "extractor/restriction.hpp"
#include "util/typedefs.hpp"

#include <boost/unordered_map.hpp>
#include <vector>

namespace osrm
{
namespace extractor
{

struct NodeRestriction;
struct TurnRestriction;

// OSRM stores restrictions in the form node -> node -> node instead of way -> node -> way (or
// way->way->way) as it is done in OSM. These restrictions need to match the state of graph
// compression which we perform in the graph compressor that removes certain degree two nodes from
// the graph (all but the ones with penalties/barriers, as of the state of writing).
// Since this graph compression ins performed after creating the restrictions in the extraction
// phase, we need to update the involved nodes whenever one of the nodes is compressed.
//
//
// !!!! Will bind to the restrictions vector and modify it in-place !!!!
class RestrictionCompressor
{
  public:
    RestrictionCompressor(std::vector<TurnRestriction> &restrictions,
                          std::vector<ConditionalTurnRestriction> &conditional_turn_restrictions,
                          std::vector<UnresolvedManeuverOverride> &maneuver_overrides);

    // account for the compression of `from-via-to` into `from-to`
    void Compress(const NodeID from, const NodeID via, const NodeID to);

  private:
    // a turn restriction is given as `from star via node to end`. Edges ending at `head` being
    // contracted move the head pointer to their respective head. Edges starting at tail move the
    // tail values to their respective tails. Way turn restrictions are represented by two
    // node-restrictions, so we can focus on them alone
    boost::unordered_multimap<NodeID, NodeRestriction *> starts;
    boost::unordered_multimap<NodeID, NodeRestriction *> ends;

    boost::unordered_multimap<NodeID, NodeBasedTurn *> maneuver_starts;
    boost::unordered_multimap<NodeID, NodeBasedTurn *> maneuver_ends;
};

} // namespace extractor
} // namespace osrm

#endif // OSRM_EXTRACTOR_RESTRICTION_COMPRESSOR_HPP_
