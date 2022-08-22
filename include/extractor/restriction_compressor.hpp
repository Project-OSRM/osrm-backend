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

// OSRM stores restrictions as node -> [node] -> node instead of way -> node -> way (or
// way->[way]->way) as it is done in OSM. These restrictions need to match the state of graph
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
                          std::vector<UnresolvedManeuverOverride> &maneuver_overrides);

    // account for the compression of `from-via-to` into `from-to`
    void Compress(const NodeID from, const NodeID via, const NodeID to);

  private:
    // A turn restriction is given as `from star via node to end`. Edges ending at `head` being
    // contracted move the head pointer to their respective head. Edges starting at tail move the
    // tail values to their respective tails.
    // Via nodes that are compressed are removed from the restriction representation.
    // We do not compress the first and last via nodes of a restriction as they act as
    // entrance/exit points into the restriction graph. For a node restriction, the first and last
    // via nodes are the same.
    boost::unordered_multimap<NodeID, TurnRestriction *> starts;
    boost::unordered_multimap<NodeID, TurnRestriction *> vias;
    boost::unordered_multimap<NodeID, TurnRestriction *> ends;

    boost::unordered_multimap<NodeID, NodeBasedTurn *> maneuver_starts;
    boost::unordered_multimap<NodeID, NodeBasedTurn *> maneuver_ends;
};

} // namespace extractor
} // namespace osrm

#endif // OSRM_EXTRACTOR_RESTRICTION_COMPRESSOR_HPP_
