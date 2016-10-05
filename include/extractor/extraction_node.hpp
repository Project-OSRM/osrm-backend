#ifndef EXTRACTION_NODE_HPP
#define EXTRACTION_NODE_HPP

#include <cstdint>

namespace osrm
{
namespace extractor
{

// Stop Signs tagged on nodes can be present or not. In addition Stop Signs have
// an optional way direction they apply to. If the direction is unknown from the
// data we have to compute by checking the distance to the next intersection.
//
// Impl. detail: namespace + enum instead of enum class to make Luabind happy
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
namespace GiveWay
{
enum State : std::uint8_t
{
    No = 1 << 0,
    YesUnknownDirection = 1 << 1,
    YesForward = 1 << 2,
    YesBackward = 1 << 3,
};
}

struct ExtractionNode
{
    ExtractionNode()
        : traffic_lights(false), barrier(false), stop_sign(StopSign::No), give_way(GiveWay::No)
    {
    }

    void clear()
    {
        traffic_lights = false;
        barrier = false;
        stop_sign = StopSign::No;
        give_way = GiveWay::No;
    }

    bool traffic_lights;
    bool barrier;

    StopSign::State stop_sign;
    GiveWay::State give_way;
};
}
}

#endif // EXTRACTION_NODE_HPP
