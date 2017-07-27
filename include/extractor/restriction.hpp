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

// A restriction that uses a single via-way in between
//
// f - e - d
//     |
// a - b - c
//
// ab via be to ef -- no u turn
struct InputWayRestriction
{
    OSMWayID from;
    OSMWayID via;
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
struct InputConditionalTurnRestriction : InputTurnRestriction
{
    std::vector<util::OpeningHours> condition;
};

// OSRM manages restrictions based on node IDs which refer to the last node along the edge. Note
// that this has the side-effect of not allowing parallel edges!
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
struct WayRestriction
{
    // a way restriction in OSRM is essentially a dual node turn restriction;
    //
    // |     |
    // c -x- b
    // |     |
    // d     a
    //
    // from ab via bxc to cd: no_uturn
    //
    // Technically, we would need only a,b,c,d to describe the full turn in terms of nodes. When
    // parsing the relation, though, we do not know about the final representation in the node-based
    // graph for the restriction. In case of a traffic light, for example, we might end up with bxc
    // not being compressed to bc. For that reason, we need to maintain two node restrictions in
    // case a way restrction is not fully collapsed
    NodeRestriction in_restriction;
    NodeRestriction out_restriction;

    bool operator==(const WayRestriction &other) const
    {
        return std::tie(in_restriction, out_restriction) ==
               std::tie(other.in_restriction, other.out_restriction);
    }
};

// Wrapper for turn restrictions that gives more information on its type / handles the switch
// between node/way/multi-way restrictions
struct TurnRestriction
{
    // keep in the same order as the turn restrictions above
    mapbox::util::variant<NodeRestriction, WayRestriction> node_or_way;
    bool is_only;

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
        node_or_way = NodeRestriction{SPECIAL_EDGEID, SPECIAL_NODEID, SPECIAL_EDGEID};
    }

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
            return restriction.in_restriction.Valid() && restriction.out_restriction.Valid();
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

struct ConditionalTurnRestriction : TurnRestriction
{
    std::vector<util::OpeningHours> condition;
};
}
}

#endif // RESTRICTION_HPP
