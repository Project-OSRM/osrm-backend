#ifndef OSRM_GUIDANCE_TURN_INSTRUCTION_HPP_
#define OSRM_GUIDANCE_TURN_INSTRUCTION_HPP_

#include <cstdint>

#include <boost/assert.hpp>

#include "extractor/guidance/roundabout_type.hpp"

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
    Invalid,                            // no valid turn instruction
    NewName,                            // no turn, but name changes
    Continue,                           // remain on a street
    Turn,                               // basic turn
    Merge,                              // merge onto a street
    Ramp,                               // special turn (highway ramp exits)
    Fork,                               // fork road splitting up
    EndOfRoad,                          // T intersection
    Notification,                       // Travel Mode Changes, Restrictions apply...
    EnterRoundabout,                    // Entering a small Roundabout
    EnterAndExitRoundabout,             // Touching a roundabout
    EnterRotary,                        // Enter a rotary
    EnterAndExitRotary,                 // Touching a rotary
    EnterRoundaboutIntersection,        // Entering a small Roundabout
    EnterAndExitRoundaboutIntersection, // Touching a roundabout
    NoTurn,                             // end of segment without turn/middle of a segment
    Suppressed,                         // location that suppresses a turn
    EnterRoundaboutAtExit,              // Entering a small Roundabout at a countable exit
    ExitRoundabout,                     // Exiting a small Roundabout
    EnterRotaryAtExit,                  // Enter A Rotary at a countable exit
    ExitRotary,                         // Exit a rotary
    EnterRoundaboutIntersectionAtExit,  // Entering a small Roundabout at a countable exit
    ExitRoundaboutIntersection,         // Exiting a small Roundabout
    StayOnRoundabout                    // Continue on Either a small or a large Roundabout
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

    static TurnInstruction REMAIN_ROUNDABOUT(const RoundaboutType, const DirectionModifier modifier)
    {
        return TurnInstruction(TurnType::StayOnRoundabout, modifier);
    }

    static TurnInstruction ENTER_ROUNDABOUT(const RoundaboutType roundabout_type,
                                            const DirectionModifier modifier)
    {
        const constexpr TurnType enter_instruction[] = {
            TurnType::Invalid, TurnType::EnterRoundabout, TurnType::EnterRotary,
            TurnType::EnterRoundaboutIntersection};
        return {enter_instruction[static_cast<int>(roundabout_type)], modifier};
    }

    static TurnInstruction EXIT_ROUNDABOUT(const RoundaboutType roundabout_type,
                                           const DirectionModifier modifier)
    {
        const constexpr TurnType exit_instruction[] = {TurnType::Invalid, TurnType::ExitRoundabout,
                                                       TurnType::ExitRotary,
                                                       TurnType::ExitRoundaboutIntersection};
        return {exit_instruction[static_cast<int>(roundabout_type)], modifier};
    }

    static TurnInstruction ENTER_AND_EXIT_ROUNDABOUT(const RoundaboutType roundabout_type,
                                                     const DirectionModifier modifier)
    {
        const constexpr TurnType exit_instruction[] = {
            TurnType::Invalid, TurnType::EnterAndExitRoundabout, TurnType::EnterAndExitRotary,
            TurnType::EnterAndExitRoundaboutIntersection};
        return {exit_instruction[static_cast<int>(roundabout_type)], modifier};
    }

    static TurnInstruction ENTER_ROUNDABOUT_AT_EXIT(const RoundaboutType roundabout_type,
                                                    const DirectionModifier modifier)
    {
        const constexpr TurnType enter_instruction[] = {
            TurnType::Invalid, TurnType::EnterRoundaboutAtExit, TurnType::EnterRotaryAtExit,
            TurnType::EnterRoundaboutIntersectionAtExit};
        return {enter_instruction[static_cast<int>(roundabout_type)], modifier};
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
