#ifndef RESTRICTION_HPP
#define RESTRICTION_HPP

#include "util/coordinate.hpp"
#include "util/opening_hours.hpp"
#include "util/typedefs.hpp"

#include "mapbox/variant.hpp"
#include <limits>

namespace osrm
{
namespace extractor
{

// OSM offers two types of restrictions, via node and via-way restrictions. We parse both into the
// same input container
//
// A restriction turning at a single node. This is the most common type of restriction:
//
// a - b - c
//     |
//     d
//
// ab via b to bd
struct InputNodeRestriction
{
    OSMWayID from;
    OSMNodeID via;
    OSMWayID to;
};

// A restriction that uses one or more via-way in between
//
// e - f - g
//     |
//     d
//     |
// a - b - c
//
// ab via bd,df to fe -- no u turn
struct InputWayRestriction
{
    OSMWayID from;
    std::vector<OSMWayID> via;
    OSMWayID to;
};

// Outside view of the variant, these are equal to the `which()` results
enum RestrictionType
{
    NODE_RESTRICTION = 0,
    WAY_RESTRICTION = 1,
    NUM_RESTRICTION_TYPES = 2
};

struct InputTurnRestriction
{
    // keep in the same order as the turn restrictions below
    mapbox::util::variant<InputNodeRestriction, InputWayRestriction> node_or_way;
    bool is_only;
    // We represent conditional and unconditional restrictions with the same structure.
    // Unconditional restrictions will have empty conditions.
    std::vector<util::OpeningHours> condition;

    OSMWayID From() const
    {
        return node_or_way.which() == RestrictionType::NODE_RESTRICTION
                   ? mapbox::util::get<InputNodeRestriction>(node_or_way).from
                   : mapbox::util::get<InputWayRestriction>(node_or_way).from;
    }

    OSMWayID To() const
    {
        return node_or_way.which() == RestrictionType::NODE_RESTRICTION
                   ? mapbox::util::get<InputNodeRestriction>(node_or_way).to
                   : mapbox::util::get<InputWayRestriction>(node_or_way).to;
    }

    RestrictionType Type() const
    {
        BOOST_ASSERT(node_or_way.which() < RestrictionType::NUM_RESTRICTION_TYPES);
        return static_cast<RestrictionType>(node_or_way.which());
    }

    InputWayRestriction &AsWayRestriction()
    {
        BOOST_ASSERT(node_or_way.which() == RestrictionType::WAY_RESTRICTION);
        return mapbox::util::get<InputWayRestriction>(node_or_way);
    }

    const InputWayRestriction &AsWayRestriction() const
    {
        BOOST_ASSERT(node_or_way.which() == RestrictionType::WAY_RESTRICTION);
        return mapbox::util::get<InputWayRestriction>(node_or_way);
    }

    InputNodeRestriction &AsNodeRestriction()
    {
        BOOST_ASSERT(node_or_way.which() == RestrictionType::NODE_RESTRICTION);
        return mapbox::util::get<InputNodeRestriction>(node_or_way);
    }

    const InputNodeRestriction &AsNodeRestriction() const
    {
        BOOST_ASSERT(node_or_way.which() == RestrictionType::NODE_RESTRICTION);
        return mapbox::util::get<InputNodeRestriction>(node_or_way);
    }
};

// OSRM manages restrictions based on node IDs which refer to the last node along the edge. Note
// that this has the side-effect of not allowing parallel edges!
//
// a - b - c
//     |
//     d
//
// ab via b to bd
struct NodeRestriction
{
    NodeID from;
    NodeID via;
    NodeID to;

    // check if all parts of the restriction reference an actual node
    bool Valid() const
    {
        return from != SPECIAL_NODEID && to != SPECIAL_NODEID && via != SPECIAL_NODEID;
    };

    bool operator==(const NodeRestriction &other) const
    {
        return std::tie(from, via, to) == std::tie(other.from, other.via, other.to);
    }
};

// A way restriction in the context of OSRM requires translation into NodeIDs. This is due to the
// compression happening in the graph creation process which would make it difficult to track
// way-ids over a series of operations. Having access to the nodes directly allows look-up of the
// edges in the processed structures
//
// e - f - g
//     |
//     d
//     |
// a - b - c
//
// ab via bd,df to fe -- no u turn
struct WayRestriction
{
    // A way restriction in OSRM needs to track all nodes that make up the via ways. Whilst most
    // of these nodes will be removed by compression, some nodes will contain features that need to
    // be considered when routing (e.g. intersections, nested restrictions, etc).
    NodeID from;
    std::vector<NodeID> via;
    NodeID to;

    // check if all parts of the restriction reference an actual node
    bool Valid() const
    {
        return from != SPECIAL_NODEID && to != SPECIAL_NODEID && via.size() >= 2 &&
               std::all_of(via.begin(), via.end(), [](NodeID i) { return i != SPECIAL_NODEID; });
    };

    bool operator==(const WayRestriction &other) const
    {
        return std::tie(from, via, to) == std::tie(other.from, other.via, other.to);
    }
};

// Wrapper for turn restrictions that gives more information on its type / handles the switch
// between node/way restrictions
struct TurnRestriction
{
    // keep in the same order as the turn restrictions above
    mapbox::util::variant<NodeRestriction, WayRestriction> node_or_way;
    bool is_only;
    // We represent conditional and unconditional restrictions with the same structure.
    // Unconditional restrictions will have empty conditions.
    std::vector<util::OpeningHours> condition;

    // construction for NodeRestrictions
    explicit TurnRestriction(NodeRestriction node_restriction, bool is_only = false)
        : node_or_way(node_restriction), is_only(is_only)
    {
    }

    // construction for WayRestrictions
    explicit TurnRestriction(WayRestriction way_restriction, bool is_only = false)
        : node_or_way(way_restriction), is_only(is_only)
    {
    }

    explicit TurnRestriction()
    {
        node_or_way = NodeRestriction{SPECIAL_NODEID, SPECIAL_NODEID, SPECIAL_NODEID};
    }

    NodeID To() const
    {
        return node_or_way.which() == RestrictionType::NODE_RESTRICTION
                   ? mapbox::util::get<NodeRestriction>(node_or_way).to
                   : mapbox::util::get<WayRestriction>(node_or_way).to;
    }

    NodeID From() const
    {
        return node_or_way.which() == RestrictionType::NODE_RESTRICTION
                   ? mapbox::util::get<NodeRestriction>(node_or_way).from
                   : mapbox::util::get<WayRestriction>(node_or_way).from;
    }

    NodeID FirstVia() const
    {
        if (node_or_way.which() == RestrictionType::NODE_RESTRICTION)
        {
            return mapbox::util::get<NodeRestriction>(node_or_way).via;
        }
        else
        {
            BOOST_ASSERT(!mapbox::util::get<WayRestriction>(node_or_way).via.empty());
            return mapbox::util::get<WayRestriction>(node_or_way).via[0];
        }
    }

    bool IsTurnRestricted(NodeID to) const { return is_only ? To() != to : To() == to; }

    bool IsUnconditional() const { return condition.empty(); }

    WayRestriction &AsWayRestriction()
    {
        BOOST_ASSERT(node_or_way.which() == RestrictionType::WAY_RESTRICTION);
        return mapbox::util::get<WayRestriction>(node_or_way);
    }

    const WayRestriction &AsWayRestriction() const
    {
        BOOST_ASSERT(node_or_way.which() == RestrictionType::WAY_RESTRICTION);
        return mapbox::util::get<WayRestriction>(node_or_way);
    }

    NodeRestriction &AsNodeRestriction()
    {
        BOOST_ASSERT(node_or_way.which() == RestrictionType::NODE_RESTRICTION);
        return mapbox::util::get<NodeRestriction>(node_or_way);
    }

    const NodeRestriction &AsNodeRestriction() const
    {
        BOOST_ASSERT(node_or_way.which() == RestrictionType::NODE_RESTRICTION);
        return mapbox::util::get<NodeRestriction>(node_or_way);
    }

    RestrictionType Type() const
    {
        BOOST_ASSERT(node_or_way.which() < RestrictionType::NUM_RESTRICTION_TYPES);
        return static_cast<RestrictionType>(node_or_way.which());
    }

    // check if all elements of the edge are considered valid
    bool Valid() const
    {
        if (node_or_way.which() == RestrictionType::WAY_RESTRICTION)
        {
            auto const &restriction = AsWayRestriction();
            return restriction.Valid();
        }
        else
        {
            auto const &restriction = AsNodeRestriction();
            return restriction.Valid();
        }
    }

    bool operator==(const TurnRestriction &other) const
    {
        if (is_only != other.is_only)
            return false;

        if (Type() != other.Type())
            return false;

        if (Type() == RestrictionType::WAY_RESTRICTION)
        {
            return AsWayRestriction() == other.AsWayRestriction();
        }
        else
        {
            return AsNodeRestriction() == other.AsNodeRestriction();
        }
    }
};
} // namespace extractor
} // namespace osrm

#endif // RESTRICTION_HPP
