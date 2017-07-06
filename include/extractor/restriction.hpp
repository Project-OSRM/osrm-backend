#ifndef RESTRICTION_HPP
#define RESTRICTION_HPP

#include "util/coordinate.hpp"
#include "util/opening_hours.hpp"
#include "util/typedefs.hpp"

#include <boost/variant.hpp>
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
    OSMEdgeID_weak from;
    OSMNodeID_weak via;
    OSMEdgeID_weak to;
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
    OSMEdgeID_weak from;
    OSMEdgeID_weak via;
    OSMEdgeID_weak to;
};

// Outside view of the variant, these are equal to the `which()` results
enum RestrictionType
{
    NODE_RESTRICTION = 0,
    WAY_RESTRICTION = 1,
    NUM_RESTRICTION_TYPES = 2
};

namespace restriction_details
{

// currently these bits only hold an `is_only` value.
struct Bits
{ // mostly unused, initialised to false by default
    Bits() : is_only(false) {}

    bool is_only;
    // when adding more bits, consider using bitfields just as in
    // bool unused : 7;
};

} // namespace restriction

struct InputTurnRestriction
{
    // keep in the same order as the turn restrictions below
    boost::variant<InputNodeRestriction, InputWayRestriction> node_or_way;
    restriction_details::Bits flags;

    OSMEdgeID_weak From() const
    {
        return node_or_way.which() == RestrictionType::NODE_RESTRICTION
                   ? boost::get<InputNodeRestriction>(node_or_way).from
                   : boost::get<InputWayRestriction>(node_or_way).from;
    }

    OSMEdgeID_weak To() const
    {
        return node_or_way.which() == RestrictionType::NODE_RESTRICTION
                   ? boost::get<InputNodeRestriction>(node_or_way).to
                   : boost::get<InputWayRestriction>(node_or_way).to;
    }

    RestrictionType Type() const
    {
        BOOST_ASSERT(node_or_way.which() < RestrictionType::NUM_RESTRICTION_TYPES);
        return static_cast<RestrictionType>(node_or_way.which());
    }

    InputWayRestriction &AsWayRestriction()
    {
        BOOST_ASSERT(node_or_way.which() == RestrictionType::WAY_RESTRICTION);
        return boost::get<InputWayRestriction>(node_or_way);
    }

    const InputWayRestriction &AsWayRestriction() const
    {
        BOOST_ASSERT(node_or_way.which() == RestrictionType::WAY_RESTRICTION);
        return boost::get<InputWayRestriction>(node_or_way);
    }

    InputNodeRestriction &AsNodeRestriction()
    {
        BOOST_ASSERT(node_or_way.which() == RestrictionType::NODE_RESTRICTION);
        return boost::get<InputNodeRestriction>(node_or_way);
    }

    const InputNodeRestriction &AsNodeRestriction() const
    {
        BOOST_ASSERT(node_or_way.which() == RestrictionType::NODE_RESTRICTION);
        return boost::get<InputNodeRestriction>(node_or_way);
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

    std::string ToString() const
    {
        return "From " + std::to_string(from) + " via " + std::to_string(via) + " to " +
               std::to_string(to);
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
};

// Wrapper for turn restrictions that gives more information on its type / handles the switch
// between node/way/multi-way restrictions
struct TurnRestriction
{
    // keep in the same order as the turn restrictions above
    boost::variant<NodeRestriction, WayRestriction> node_or_way;
    restriction_details::Bits flags;

    // construction for NodeRestrictions
    explicit TurnRestriction(NodeRestriction node_restriction, bool is_only = false)
        : node_or_way(node_restriction)
    {
        flags.is_only = is_only;
    }

    // construction for WayRestrictions
    explicit TurnRestriction(WayRestriction way_restriction, bool is_only = false)
        : node_or_way(way_restriction)
    {
        flags.is_only = is_only;
    }

    explicit TurnRestriction()
    {
        node_or_way = NodeRestriction{SPECIAL_EDGEID, SPECIAL_NODEID, SPECIAL_EDGEID};
    }

    WayRestriction &AsWayRestriction()
    {
        BOOST_ASSERT(node_or_way.which() == RestrictionType::WAY_RESTRICTION);
        return boost::get<WayRestriction>(node_or_way);
    }

    const WayRestriction &AsWayRestriction() const
    {
        BOOST_ASSERT(node_or_way.which() == RestrictionType::WAY_RESTRICTION);
        return boost::get<WayRestriction>(node_or_way);
    }

    NodeRestriction &AsNodeRestriction()
    {
        BOOST_ASSERT(node_or_way.which() == RestrictionType::NODE_RESTRICTION);
        return boost::get<NodeRestriction>(node_or_way);
    }

    const NodeRestriction &AsNodeRestriction() const
    {
        BOOST_ASSERT(node_or_way.which() == RestrictionType::NODE_RESTRICTION);
        return boost::get<NodeRestriction>(node_or_way);
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

    std::string ToString() const
    {
        std::string representation;
        if (node_or_way.which() == RestrictionType::WAY_RESTRICTION)
        {
            auto const &way = AsWayRestriction();
            representation =
                "In: " + way.in_restriction.ToString() + " Out: " + way.out_restriction.ToString();
        }
        else
        {
            auto const &node = AsNodeRestriction();
            representation = node.ToString();
        }
        representation += " is_only: " + std::to_string(flags.is_only);
        return representation;
    }
};

struct ConditionalTurnRestriction : TurnRestriction
{
    std::vector<util::OpeningHours> condition;
};
}
}

#endif // RESTRICTION_HPP
