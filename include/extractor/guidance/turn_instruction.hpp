#ifndef OSRM_GUIDANCE_TURN_INSTRUCTION_HPP_
#define OSRM_GUIDANCE_TURN_INSTRUCTION_HPP_

#include <cstdint>

#include <boost/assert.hpp>

namespace osrm
{
namespace extractor
{
namespace guidance
{

namespace detail
{
// inclusive bounds for turn modifiers
const constexpr uint8_t num_direction_modifiers = 8;
} // detail

// direction modifiers based on angle
// Would be nice to have
// enum class DirectionModifier : unsigned char
enum DirectionModifier
{
    UTurn,
    SharpRight,
    Right,
    SlightRight,
    Straight,
    SlightLeft,
    Left,
    SharpLeft
};

// enum class TurnType : unsigned char
enum TurnType // at the moment we can support 32 turn types, without increasing memory consumption
{
    Invalid,                // no valid turn instruction
    NoTurn,                 // end of segment without turn/middle of a segment
    Suppressed,             // location that suppresses a turn
    NewName,                // no turn, but name changes
    Continue,               // remain on a street
    Turn,                   // basic turn
    Merge,                  // merge onto a street
    Ramp,                   // special turn (highway ramp exits)
    Fork,                   // fork road splitting up
    EndOfRoad,              // T intersection
    EnterRoundabout,        // Entering a small Roundabout
    EnterRoundaboutAtExit,  // Entering a small Roundabout at a countable exit
    EnterAndExitRoundabout, // Touching a roundabout
    ExitRoundabout,         // Exiting a small Roundabout
    EnterRotary,            // Enter a rotary
    EnterRotaryAtExit,      // Enter A Rotary at a countable exit
    EnterAndExitRotary,     // Touching a rotary
    ExitRotary,             // Exit a rotary
    StayOnRoundabout,       // Continue on Either a small or a large Roundabout
    Notification            // Travel Mode Changes`
};

// turn angle in 1.40625 degree -> 128 == 180 degree
struct TurnInstruction
{
    TurnInstruction(const TurnType type = TurnType::Invalid,
                    const DirectionModifier direction_modifier = DirectionModifier::Straight)
        : type(type), direction_modifier(direction_modifier)
    {
    }

    TurnType type : 5;
    DirectionModifier direction_modifier : 3;

    static TurnInstruction INVALID()
    {
        return TurnInstruction(TurnType::Invalid, DirectionModifier::UTurn);
    }

    static TurnInstruction NO_TURN()
    {
        return TurnInstruction(TurnType::NoTurn, DirectionModifier::UTurn);
    }

    static TurnInstruction REMAIN_ROUNDABOUT(bool is_rotary, const DirectionModifier modifier)
    {
        (void)is_rotary; // staying does not require a different instruction at the moment
        return TurnInstruction(TurnType::StayOnRoundabout, modifier);
    }

    static TurnInstruction ENTER_ROUNDABOUT(bool is_rotary, const DirectionModifier modifier)
    {
        return {is_rotary ? TurnType::EnterRotary : TurnType::EnterRoundabout, modifier};
    }

    static TurnInstruction EXIT_ROUNDABOUT(bool is_rotary, const DirectionModifier modifier)
    {
        return {is_rotary ? TurnType::ExitRotary : TurnType::ExitRoundabout, modifier};
    }

    static TurnInstruction ENTER_AND_EXIT_ROUNDABOUT(bool is_rotary,
                                                     const DirectionModifier modifier)
    {
        return {is_rotary ? TurnType::EnterAndExitRotary : TurnType::EnterAndExitRoundabout,
                modifier};
    }

    static TurnInstruction SUPPRESSED(const DirectionModifier modifier)
    {
        return {TurnType::Suppressed, modifier};
    }
};

inline bool operator!=(const TurnInstruction lhs, const TurnInstruction rhs)
{
    return lhs.type != rhs.type || lhs.direction_modifier != rhs.direction_modifier;
}

inline bool operator==(const TurnInstruction lhs, const TurnInstruction rhs)
{
    return lhs.type == rhs.type && lhs.direction_modifier == rhs.direction_modifier;
}

} // namespace guidance
} // namespace extractor
} // namespace osrm

#endif // OSRM_GUIDANCE_TURN_INSTRUCTION_HPP_
