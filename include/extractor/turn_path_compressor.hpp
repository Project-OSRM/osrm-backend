#ifndef OSRM_EXTRACTOR_TURN_PATH_COMPRESSOR_HPP_
#define OSRM_EXTRACTOR_TURN_PATH_COMPRESSOR_HPP_

#include "util/typedefs.hpp"

#include <unordered_map>
#include <vector>

namespace osrm::extractor
{

struct TurnPath;
struct TurnRestriction;
struct UnresolvedManeuverOverride;

// OSRM stores turn paths as node -> [node] -> node instead of way -> node -> way (or
// way->[way]->way) as it is done in OSM. These paths need to match the state of graph
// compression which we perform in the graph compressor that removes certain degree two nodes from
// the graph (all but the ones with penalties/barriers, as of the state of writing).
// Since this graph compression is performed after creating the turn paths in the extraction
// phase, we need to update the involved nodes whenever one of the nodes is compressed.
//
//
// !!!! Will bind to the restriction/maneuver vectors and modify it in-place !!!!
class TurnPathCompressor
{
  public:
    TurnPathCompressor(std::vector<TurnRestriction> &restrictions,
                       std::vector<UnresolvedManeuverOverride> &maneuver_overrides);

    // account for the compression of `from-via-to` into `from-to`
    void Compress(const NodeID from, const NodeID via, const NodeID to);

  private:
    // A turn path is given as `from start via node(s) to end`. Edges ending at `head` being
    // contracted move the head pointer to their respective head. Edges starting at tail move the
    // tail values to their respective tails.
    // Via nodes that are compressed are removed from the restriction representation.
    // We do not compress the first and last via nodes of a restriction as they act as
    // entrance/exit points into the restriction graph. For a node restriction, the first and last
    // via nodes are the same.
    // Similarly, we do not compress the instruction via node in a maneuver override, as we need
    // this to identify the location of the maneuver during routing path-processing.
    std::unordered_multimap<NodeID, TurnPath *> starts;
    std::unordered_multimap<NodeID, TurnPath *> vias;
    std::unordered_multimap<NodeID, TurnPath *> ends;
};

} // namespace osrm::extractor

#endif // OSRM_EXTRACTOR_TURN_PATH_COMPRESSOR_HPP_
