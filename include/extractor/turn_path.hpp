#ifndef OSRM_TURN_PATH_HPP
#define OSRM_TURN_PATH_HPP

#include "util/typedefs.hpp"

#include <algorithm>
#include <variant>
#include <vector>

namespace osrm::extractor
{

// Outside view of the variant, these are equal to the `which()` results
enum TurnPathType
{
    VIA_NODE_TURN_PATH = 0,
    VIA_WAY_TURN_PATH = 1,
    NUM_TURN_PATH_TYPES = 2
};

// OSM turn restrictions and maneuver overrides are relations that use the same path
// representation. Therefore, we can represent these paths by a shared, common structure.
//
// from: the way from which the turn sequence begins
// via: a node or list of ways, representing the intermediate path taken in the turn sequence
// to: the final way in the turn sequence
//
// We will have two representations of the paths, via-node and via-way, to represent the two options
// for the intermediate path. We parse both into the same input container

//
// A path turning at a single node. This is the most common type of relation:
//
// a - b - c
//     |
//     d
//
// ab via b to bd
struct InputViaNodePath
{
    OSMWayID from;
    OSMNodeID via;
    OSMWayID to;
};

// A turn path that uses one or more via-way in between
//
// e - f - g
//     |
//     d
//     |
// a - b - c
//
// ab via bd,df to fe
struct InputViaWayPath
{
    OSMWayID from;
    std::vector<OSMWayID> via;
    OSMWayID to;
};

struct InputTurnPath
{
    std::variant<InputViaNodePath, InputViaWayPath> node_or_way;

    TurnPathType Type() const
    {
        BOOST_ASSERT(node_or_way.index() < TurnPathType::NUM_TURN_PATH_TYPES);
        return static_cast<TurnPathType>(node_or_way.index());
    }

    OSMWayID From() const
    {
        return node_or_way.index() == TurnPathType::VIA_NODE_TURN_PATH
                   ? std::get<InputViaNodePath>(node_or_way).from
                   : std::get<InputViaWayPath>(node_or_way).from;
    }

    OSMWayID To() const
    {
        return node_or_way.index() == TurnPathType::VIA_NODE_TURN_PATH
                   ? std::get<InputViaNodePath>(node_or_way).to
                   : std::get<InputViaWayPath>(node_or_way).to;
    }

    InputViaWayPath &AsViaWayPath()
    {
        BOOST_ASSERT(node_or_way.index() == TurnPathType::VIA_WAY_TURN_PATH);
        return std::get<InputViaWayPath>(node_or_way);
    }

    const InputViaWayPath &AsViaWayPath() const
    {
        BOOST_ASSERT(node_or_way.index() == TurnPathType::VIA_WAY_TURN_PATH);
        return std::get<InputViaWayPath>(node_or_way);
    }

    InputViaNodePath &AsViaNodePath()
    {
        BOOST_ASSERT(node_or_way.index() == TurnPathType::VIA_NODE_TURN_PATH);
        return std::get<InputViaNodePath>(node_or_way);
    }

    const InputViaNodePath &AsViaNodePath() const
    {
        BOOST_ASSERT(node_or_way.index() == TurnPathType::VIA_NODE_TURN_PATH);
        return std::get<InputViaNodePath>(node_or_way);
    }
};

// Internally, we convert the turn paths into a node-based-node representation.
// This allows us to correctly track the edges as they processed, such as during graph compression.
// Having access to the nodes directly allows look-up of the edges in the processed structures,
// and can be utilised during edge-based-graph generation.
//
// Once again, we keep two representations of the paths, via-node and via-way, for more efficient
// representation of the more common via-node path.
//
// a - b - c
//     |
//     d
//
// a via b to d
struct ViaNodePath
{
    NodeID from;
    NodeID via;
    NodeID to;

    // check if all parts of the restriction reference an actual node
    bool Valid() const
    {
        return from != SPECIAL_NODEID && to != SPECIAL_NODEID && via != SPECIAL_NODEID;
    };

    bool operator==(const ViaNodePath &other) const
    {
        return std::tie(from, via, to) == std::tie(other.from, other.via, other.to);
    }
};

//
//
// e - f - g
//     |
//     d
//     |
// a - b - c
//
// a via bdf to e
// (after compression) a via bf to e
struct ViaWayPath
{
    // A way path in OSRM needs to track all nodes that make up the via ways. Whilst most
    // of these nodes will be removed by compression, some nodes will contain features that need to
    // be considered when routing (e.g. intersections, nested restrictions, etc).
    NodeID from;
    std::vector<NodeID> via;
    NodeID to;

    // check if all parts of the path reference an actual node
    bool Valid() const
    {
        return from != SPECIAL_NODEID && to != SPECIAL_NODEID && via.size() >= 2 &&
               std::all_of(via.begin(), via.end(), [](NodeID i) { return i != SPECIAL_NODEID; });
    };

    bool operator==(const ViaWayPath &other) const
    {
        return std::tie(from, via, to) == std::tie(other.from, other.via, other.to);
    }
};

// Wrapper for turn paths that gives more information on its type / handles the switch
// between node/way paths
struct TurnPath
{
    std::variant<ViaNodePath, ViaWayPath> node_or_way;

    NodeID To() const
    {
        return node_or_way.index() == TurnPathType::VIA_NODE_TURN_PATH
                   ? std::get<ViaNodePath>(node_or_way).to
                   : std::get<ViaWayPath>(node_or_way).to;
    }

    NodeID From() const
    {
        return node_or_way.index() == TurnPathType::VIA_NODE_TURN_PATH
                   ? std::get<ViaNodePath>(node_or_way).from
                   : std::get<ViaWayPath>(node_or_way).from;
    }

    NodeID FirstVia() const
    {
        if (node_or_way.index() == TurnPathType::VIA_NODE_TURN_PATH)
        {
            return std::get<ViaNodePath>(node_or_way).via;
        }
        else
        {
            BOOST_ASSERT(!std::get<ViaWayPath>(node_or_way).via.empty());
            return std::get<ViaWayPath>(node_or_way).via[0];
        }
    }

    ViaWayPath &AsViaWayPath()
    {
        BOOST_ASSERT(node_or_way.index() == TurnPathType::VIA_WAY_TURN_PATH);
        return std::get<ViaWayPath>(node_or_way);
    }

    const ViaWayPath &AsViaWayPath() const
    {
        BOOST_ASSERT(node_or_way.index() == TurnPathType::VIA_WAY_TURN_PATH);
        return std::get<ViaWayPath>(node_or_way);
    }

    ViaNodePath &AsViaNodePath()
    {
        BOOST_ASSERT(node_or_way.index() == TurnPathType::VIA_NODE_TURN_PATH);
        return std::get<ViaNodePath>(node_or_way);
    }

    const ViaNodePath &AsViaNodePath() const
    {
        BOOST_ASSERT(node_or_way.index() == TurnPathType::VIA_NODE_TURN_PATH);
        return std::get<ViaNodePath>(node_or_way);
    }

    TurnPathType Type() const
    {
        BOOST_ASSERT(node_or_way.index() < TurnPathType::NUM_TURN_PATH_TYPES);
        return static_cast<TurnPathType>(node_or_way.index());
    }

    bool operator==(const TurnPath &other) const
    {
        if (Type() != other.Type())
            return false;

        if (Type() == TurnPathType::VIA_WAY_TURN_PATH)
        {
            return AsViaWayPath() == other.AsViaWayPath();
        }
        else
        {
            return AsViaNodePath() == other.AsViaNodePath();
        }
    }

    bool Valid() const
    {
        if (Type() == TurnPathType::VIA_WAY_TURN_PATH)
        {
            return AsViaWayPath().Valid();
        }
        else
        {
            return AsViaNodePath().Valid();
        }
    };
};

} // namespace osrm::extractor
#endif // OSRM_TURN_PATH_HPP
