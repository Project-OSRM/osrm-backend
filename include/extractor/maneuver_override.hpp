#ifndef MANUEVER_OVERRIDE_HPP
#define MANUEVER_OVERRIDE_HPP

#include "guidance/turn_instruction.hpp"
#include "util/typedefs.hpp"

#include "storage/shared_memory_ownership.hpp"
#include "util/vector_view.hpp"
#include <algorithm>
#include <boost/functional/hash.hpp>

namespace osrm
{
namespace extractor
{

// Data that is loaded from the OSM datafile directly
struct InputManeuverOverride
{
    std::vector<OSMWayID> via_ways;
    OSMNodeID via_node;
    std::string maneuver;
    std::string direction;
};

// Object returned by the datafacade
struct ManeuverOverride
{
    // util::ViewOrVector<NodeID, storage::Ownership::View> node_sequence;
    std::vector<NodeID> node_sequence;
    // before the turn, then later, the edge_based_node_id of the turn
    NodeID instruction_node; // node-based node ID
    guidance::TurnType::Enum override_type;
    guidance::DirectionModifier::Enum direction;
};

// Object returned by the datafacade
struct StorageManeuverOverride
{
    std::uint32_t node_sequence_offset_begin;
    std::uint32_t node_sequence_offset_end;
    NodeID start_node;
    // before the turn, then later, the edge_based_node_id of the turn
    NodeID instruction_node; // node-based node ID
    guidance::TurnType::Enum override_type;
    guidance::DirectionModifier::Enum direction;
};

struct NodeBasedTurn
{
    NodeID from;
    NodeID via;
    NodeID to;

    bool operator==(const NodeBasedTurn &other) const
    {
        return other.from == from && other.via == via && other.to == to;
    }
};

struct UnresolvedManeuverOverride
{

    std::vector<NodeBasedTurn>
        turn_sequence; // initially the internal node-based-node ID of the node
    // before the turn, then later, the edge_based_node_id of the turn
    NodeID instruction_node; // node-based node ID
    guidance::TurnType::Enum override_type;
    guidance::DirectionModifier::Enum direction;

    // check if all parts of the restriction reference an actual node
    bool Valid() const
    {
        return !turn_sequence.empty() &&
               std::none_of(turn_sequence.begin(),
                            turn_sequence.end(),
                            [](const auto &n) {
                                return n.from == SPECIAL_NODEID || n.via == SPECIAL_NODEID ||
                                       n.to == SPECIAL_NODEID;
                            }) &&
               (direction != guidance::DirectionModifier::MaxDirectionModifier ||
                override_type != guidance::TurnType::Invalid);
    }
};
} // namespace extractor
} // namespace osrm

// custom specialization of std::hash can be injected in namespace std
namespace std
{
template <> struct hash<osrm::extractor::NodeBasedTurn>

{
    typedef osrm::extractor::NodeBasedTurn argument_type;
    typedef std::size_t result_type;
    result_type operator()(argument_type const &s) const noexcept
    {

        std::size_t seed = 0;
        boost::hash_combine(seed, s.from);
        boost::hash_combine(seed, s.via);
        boost::hash_combine(seed, s.to);

        return seed;
    }
};
} // namespace std

#endif
/*
from=1
to=3
via=b

      101      a      102      b        103
---------------+---------------+-------------- (way 1)
      99        \      98       \       97
             51  \ 2          50 \ 3
                  \               \
*/
