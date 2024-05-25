#ifndef RESTRICTION_HPP
#define RESTRICTION_HPP

#include "turn_path.hpp"
#include "util/coordinate.hpp"
#include "util/opening_hours.hpp"
#include "util/typedefs.hpp"
#include <limits>

namespace osrm::extractor
{

// External (OSM) representation of restriction
struct InputTurnRestriction
{
    InputTurnPath turn_path;
    bool is_only;
    std::vector<util::OpeningHours> condition;
};

// Internal (OSRM) representation of restriction
struct TurnRestriction
{
    // The turn sequence that the restriction applies to.
    TurnPath turn_path;
    // Indicates if the restriction turn *must* or *must not* be taken.
    bool is_only = false;
    // We represent conditional and unconditional restrictions with the same structure.
    // Unconditional restrictions will have empty conditions.
    std::vector<util::OpeningHours> condition;

    explicit TurnRestriction(const TurnPath &turn_path, bool is_only = false)
        : turn_path(turn_path), is_only(is_only)
    {
    }

    explicit TurnRestriction()
    {
        turn_path = {ViaNodePath{SPECIAL_NODEID, SPECIAL_NODEID, SPECIAL_NODEID}};
    }

    bool IsTurnRestricted(NodeID to) const
    {
        return is_only ? turn_path.To() != to : turn_path.To() == to;
    }

    bool IsUnconditional() const { return condition.empty(); }

    // check if all elements of the edge are considered valid
    bool Valid() const { return turn_path.Valid(); }

    bool operator==(const TurnRestriction &other) const
    {
        if (is_only != other.is_only)
            return false;

        return turn_path == other.turn_path;
    }

    static std::string Name() { return "turn restriction"; };
};
} // namespace osrm::extractor

#endif // RESTRICTION_HPP
