#ifndef MANUEVER_OVERRIDE_HPP
#define MANUEVER_OVERRIDE_HPP

#include "guidance/turn_instruction.hpp"
#include "util/typedefs.hpp"

#include "storage/shared_memory_ownership.hpp"
#include "turn_path.hpp"
#include "util/integer_range.hpp"
#include "util/log.hpp"
#include "util/std_hash.hpp"
#include "util/vector_view.hpp"

#include <variant>

#include <algorithm>

namespace osrm::extractor
{

// Data that is loaded from the OSM datafile directly
struct InputManeuverOverride
{
    InputTurnPath turn_path;
    OSMNodeID via_node;
    std::string maneuver;
    std::string direction;
};

// Object returned by the datafacade
struct ManeuverOverride
{
    std::vector<NodeID> node_sequence;
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
    NodeID instruction_node; // node-based node ID
    guidance::TurnType::Enum override_type;
    guidance::DirectionModifier::Enum direction;
};

// Used to identify maneuver turns whilst generating edge-based graph
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

// Internal representation of maneuvers during graph extraction phase
struct UnresolvedManeuverOverride
{
    // The turn sequence that the maneuver override applies to.
    TurnPath turn_path;
    NodeID instruction_node; // node-based node ID
    guidance::TurnType::Enum override_type;
    guidance::DirectionModifier::Enum direction;

    UnresolvedManeuverOverride()
    {
        turn_path = {ViaNodePath{SPECIAL_NODEID, SPECIAL_NODEID, SPECIAL_NODEID}};
        instruction_node = SPECIAL_NODEID;
        override_type = guidance::TurnType::Invalid;
        direction = guidance::DirectionModifier::MaxDirectionModifier;
    }

    // check if all parts of the restriction reference an actual node
    bool Valid() const
    {
        if ((direction == guidance::DirectionModifier::MaxDirectionModifier &&
             override_type == guidance::TurnType::Invalid) ||
            instruction_node == SPECIAL_NODEID || !turn_path.Valid())
        {
            return false;
        }

        if (turn_path.Type() == TurnPathType::VIA_NODE_TURN_PATH)
        {
            const auto node_path = turn_path.AsViaNodePath();
            if (node_path.via != instruction_node)
            {
                util::Log(logDEBUG) << "Maneuver via-node " << node_path.via
                                    << " does not match instruction node " << instruction_node;
                return false;
            }
        }
        else
        {
            BOOST_ASSERT(turn_path.Type() == TurnPathType::VIA_WAY_TURN_PATH);
            const auto way_path = turn_path.AsViaWayPath();

            if (std::find(way_path.via.begin(), way_path.via.end(), instruction_node) ==
                way_path.via.end())
            {
                util::Log(logDEBUG) << "Maneuver via-way path does not contain instruction node "
                                    << instruction_node;
                return false;
            }
        }
        return true;
    }

    // Generate sequence of node-based-node turns. Used to identify the maneuver's edge-based-node
    // turns during graph expansion.
    std::vector<NodeBasedTurn> Turns() const
    {
        if (turn_path.Type() == TurnPathType::VIA_NODE_TURN_PATH)
        {
            const auto node_maneuver = turn_path.AsViaNodePath();
            return {{node_maneuver.from, node_maneuver.via, node_maneuver.to}};
        }

        BOOST_ASSERT(turn_path.Type() == TurnPathType::VIA_WAY_TURN_PATH);
        std::vector<NodeBasedTurn> result;
        const auto way_maneuver = turn_path.AsViaWayPath();
        BOOST_ASSERT(way_maneuver.via.size() >= 2);
        result.push_back({way_maneuver.from, way_maneuver.via[0], way_maneuver.via[1]});

        for (auto i : util::irange<size_t>(0, way_maneuver.via.size() - 2))
        {
            result.push_back(
                {way_maneuver.via[i], way_maneuver.via[i + 1], way_maneuver.via[i + 2]});
        }
        result.push_back({way_maneuver.via[way_maneuver.via.size() - 2],
                          way_maneuver.via.back(),
                          way_maneuver.to});

        return result;
    }

    static std::string Name() { return "maneuver override"; };
};
} // namespace osrm::extractor

// custom specialization of std::hash can be injected in namespace std
namespace std
{
template <> struct hash<osrm::extractor::NodeBasedTurn>
{
    using argument_type = osrm::extractor::NodeBasedTurn;
    using result_type = std::size_t;
    result_type operator()(argument_type const &s) const noexcept
    {

        std::size_t seed = 0;
        hash_combine(seed, s.from);
        hash_combine(seed, s.via);
        hash_combine(seed, s.to);

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
