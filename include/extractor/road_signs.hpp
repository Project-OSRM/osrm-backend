#ifndef OSRM_PRIORITY_ROADS_HPP
#define OSRM_PRIORITY_ROADS_HPP

#include <cstdint>

namespace osrm
{
namespace extractor
{
// Impl. detail: namespace + enum instead of enum class to make Luabind happy
// TODO: make type safe enum classes as soon as we can bind them to Lua.

// Stop Signs tagged on nodes can be present or not. In addition Stop Signs have
// an optional way direction they apply to. If the direction is unknown from the
// data we have to compute by checking the distance to the next intersection.
namespace StopSign
{
enum State : std::uint8_t
{
    No = 1 << 0,
    YesUnknownDirection = 1 << 1,
    YesForward = 1 << 2,
    YesBackward = 1 << 3,
};
}

// Give Way is the complement to priority roads. Tagging is the same as Stop Signs.
// See explanation above.
namespace GiveWaySign
{
enum State : std::uint8_t
{
    No = 1 << 0,
    YesUnknownDirection = 1 << 1,
    YesForward = 1 << 2,
    YesBackward = 1 << 3,
};
}

}
}

#endif // EXTRACTION_NODE_HPP
