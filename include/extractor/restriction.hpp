#ifndef RESTRICTION_HPP
#define RESTRICTION_HPP

#include "util/coordinate.hpp"
#include "util/typedefs.hpp"

#include <limits>

namespace osrm
{
namespace extractor
{

struct TurnRestriction
{
    union WayOrNode {
        OSMNodeID_weak node;
        OSMEdgeID_weak way;
    };
    WayOrNode via;
    WayOrNode from;
    WayOrNode to;

    std::string condition; // todo store this as something cuter than a string

    struct Bits
    { // mostly unused
        Bits()
            : is_only(false), uses_via_way(false), unused2(false), unused3(false), unused4(false),
              unused5(false), unused6(false), unused7(false)
        {
        }

        bool is_only : 1;
        bool uses_via_way : 1;
        bool unused2 : 1;
        bool unused3 : 1;
        bool unused4 : 1;
        bool unused5 : 1;
        bool unused6 : 1;
        bool unused7 : 1;
    } flags;

    explicit TurnRestriction(NodeID node)
    {
        via.node = node;
        from.node = SPECIAL_NODEID;
        to.node = SPECIAL_NODEID;
    }

    explicit TurnRestriction(const bool is_only = false)
    {
        via.node = SPECIAL_NODEID;
        from.node = SPECIAL_NODEID;
        to.node = SPECIAL_NODEID;
        flags.is_only = is_only;
    }
};

/**
 * This is just a wrapper around TurnRestriction used in the extractor.
 *
 * Could be merged with TurnRestriction. For now the type-destiction makes sense
 * as the format in which the restriction is presented in the extractor and in the
 * preprocessing is different. (see restriction_parser.cpp)
 */
struct InputRestrictionContainer
{
    TurnRestriction restriction;

    InputRestrictionContainer(EdgeID fromWay, EdgeID toWay, EdgeID vw)
    {
        restriction.from.way = fromWay;
        restriction.to.way = toWay;
        restriction.via.way = vw;
    }
    explicit InputRestrictionContainer(bool is_only = false)
    {
        restriction.from.way = SPECIAL_EDGEID;
        restriction.to.way = SPECIAL_EDGEID;
        restriction.via.node = SPECIAL_NODEID;
        restriction.flags.is_only = is_only;
    }

    static InputRestrictionContainer min_value() { return InputRestrictionContainer(0, 0, 0); }
    static InputRestrictionContainer max_value()
    {
        return InputRestrictionContainer(SPECIAL_EDGEID, SPECIAL_EDGEID, SPECIAL_EDGEID);
    }
};

struct CmpRestrictionContainerByFrom
{
    using value_type = InputRestrictionContainer;
    bool operator()(const InputRestrictionContainer &a, const InputRestrictionContainer &b) const
    {
        return a.restriction.from.way < b.restriction.from.way;
    }
    value_type max_value() const { return InputRestrictionContainer::max_value(); }
    value_type min_value() const { return InputRestrictionContainer::min_value(); }
};

struct CmpRestrictionContainerByTo
{
    using value_type = InputRestrictionContainer;
    bool operator()(const InputRestrictionContainer &a, const InputRestrictionContainer &b) const
    {
        return a.restriction.to.way < b.restriction.to.way;
    }
    value_type max_value() const { return InputRestrictionContainer::max_value(); }
    value_type min_value() const { return InputRestrictionContainer::min_value(); }
};
}
}

#endif // RESTRICTION_HPP
