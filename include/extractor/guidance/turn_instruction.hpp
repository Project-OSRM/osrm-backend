#ifndef OSRM_GUIDANCE_TURN_INSTRUCTION_HPP_
#define OSRM_GUIDANCE_TURN_INSTRUCTION_HPP_

#include <algorithm>
#include <cstdint>

#include "extractor/guidance/roundabout_type.hpp"
#include "util/attributes.hpp"
#include "util/typedefs.hpp"

namespace osrm
{
namespace extractor
{
namespace guidance
{

// direction modifiers based on angle
namespace DirectionModifier
{
typedef std::uint8_t Enum;
const constexpr Enum UTurn = 0;
const constexpr Enum SharpRight = 1;
const constexpr Enum Right = 2;
const constexpr Enum SlightRight = 3;
const constexpr Enum Straight = 4;
const constexpr Enum SlightLeft = 5;
const constexpr Enum Left = 6;
const constexpr Enum SharpLeft = 7;
const constexpr Enum MaxDirectionModifier = 8;
}

namespace TurnType
{
typedef std::uint8_t Enum;
const constexpr Enum Invalid = 0;                      // no valid turn instruction
const constexpr Enum NewName = 1;                      // no turn, but name changes
const constexpr Enum Continue = 2;                     // remain on a street
const constexpr Enum Turn = 3;                         // basic turn
const constexpr Enum Merge = 4;                        // merge onto a street
const constexpr Enum OnRamp = 5;                       // special turn (highway ramp on-ramps)
const constexpr Enum OffRamp = 6;                      // special turn, highway exit
const constexpr Enum Fork = 7;                         // fork road splitting up
const constexpr Enum EndOfRoad = 8;                    // T intersection
const constexpr Enum Notification = 9;                 // Travel Mode Changes, Restrictions apply...
const constexpr Enum EnterRoundabout = 10;             // Entering a small Roundabout
const constexpr Enum EnterAndExitRoundabout = 11;      // Touching a roundabout
const constexpr Enum EnterRotary = 12;                 // Enter a rotary
const constexpr Enum EnterAndExitRotary = 13;          // Touching a rotary
const constexpr Enum EnterRoundaboutIntersection = 14; // Entering a small Roundabout
const constexpr Enum EnterAndExitRoundaboutIntersection = 15; // Touching a roundabout
const constexpr Enum UseLane = 16; // No Turn, but you need to stay on a given lane!

// Values below here are silent instructions
const constexpr Enum NoTurn = 17;                // end of segment without turn/middle of a segment
const constexpr Enum Suppressed = 18;            // location that suppresses a turn
const constexpr Enum EnterRoundaboutAtExit = 19; // Entering a small Roundabout at a countable exit
const constexpr Enum ExitRoundabout = 20;        // Exiting a small Roundabout
const constexpr Enum EnterRotaryAtExit = 21;     // Enter A Rotary at a countable exit
const constexpr Enum ExitRotary = 22;            // Exit a rotary
const constexpr Enum EnterRoundaboutIntersectionAtExit =
    23; // Entering a small Roundabout at a countable exit
const constexpr Enum ExitRoundaboutIntersection = 24; // Exiting a small Roundabout
const constexpr Enum StayOnRoundabout = 25; // Continue on Either a small or a large Roundabout
const constexpr Enum Sliproad =
    26; // Something that looks like a ramp, but is actually just a small sliproad
const constexpr Enum MaxTurnType = 27; // Special value for static asserts
}

struct TurnInstruction
{
    TurnInstruction(const TurnType::Enum type = TurnType::Invalid,
                    const DirectionModifier::Enum direction_modifier = DirectionModifier::UTurn)
        : type(type), direction_modifier(direction_modifier)
    {
    }

    TurnType::Enum type : 5;
    DirectionModifier::Enum direction_modifier : 3;
    // the lane tupel that is used for the turn

    static TurnInstruction INVALID() { return {TurnType::Invalid, DirectionModifier::UTurn}; }

    static TurnInstruction NO_TURN() { return {TurnType::NoTurn, DirectionModifier::UTurn}; }

    static TurnInstruction REMAIN_ROUNDABOUT(const RoundaboutType,
                                             const DirectionModifier::Enum modifier)
    {
        return {TurnType::StayOnRoundabout, modifier};
    }

    static TurnInstruction ENTER_ROUNDABOUT(const RoundaboutType roundabout_type,
                                            const DirectionModifier::Enum modifier)
    {
        const constexpr TurnType::Enum enter_instruction[] = {
            TurnType::Invalid,
            TurnType::EnterRoundabout,
            TurnType::EnterRotary,
            TurnType::EnterRoundaboutIntersection};
        return {enter_instruction[static_cast<int>(roundabout_type)], modifier};
    }

    static TurnInstruction EXIT_ROUNDABOUT(const RoundaboutType roundabout_type,
                                           const DirectionModifier::Enum modifier)
    {
        const constexpr TurnType::Enum exit_instruction[] = {TurnType::Invalid,
                                                             TurnType::ExitRoundabout,
                                                             TurnType::ExitRotary,
                                                             TurnType::ExitRoundaboutIntersection};
        return {exit_instruction[static_cast<int>(roundabout_type)], modifier};
    }

    static TurnInstruction ENTER_AND_EXIT_ROUNDABOUT(const RoundaboutType roundabout_type,
                                                     const DirectionModifier::Enum modifier)
    {
        const constexpr TurnType::Enum exit_instruction[] = {
            TurnType::Invalid,
            TurnType::EnterAndExitRoundabout,
            TurnType::EnterAndExitRotary,
            TurnType::EnterAndExitRoundaboutIntersection};
        return {exit_instruction[static_cast<int>(roundabout_type)], modifier};
    }

    static TurnInstruction ENTER_ROUNDABOUT_AT_EXIT(const RoundaboutType roundabout_type,
                                                    const DirectionModifier::Enum modifier)
    {
        const constexpr TurnType::Enum enter_instruction[] = {
            TurnType::Invalid,
            TurnType::EnterRoundaboutAtExit,
            TurnType::EnterRotaryAtExit,
            TurnType::EnterRoundaboutIntersectionAtExit};
        return {enter_instruction[static_cast<int>(roundabout_type)], modifier};
    }

    static TurnInstruction SUPPRESSED(const DirectionModifier::Enum modifier)
    {
        return {TurnType::Suppressed, modifier};
    }
};

static_assert(sizeof(TurnInstruction) == 1, "TurnInstruction does not fit a byte");

inline bool operator!=(const TurnInstruction lhs, const TurnInstruction rhs)
{
    return lhs.type != rhs.type || lhs.direction_modifier != rhs.direction_modifier;
}

inline bool operator==(const TurnInstruction lhs, const TurnInstruction rhs)
{
    return lhs.type == rhs.type && lhs.direction_modifier == rhs.direction_modifier;
}

// check if a instruction is associated in any form with a roundabout
inline bool hasRoundaboutType(const TurnInstruction instruction)
{
    using namespace extractor::guidance::TurnType;
    const constexpr TurnType::Enum valid_types[] = {TurnType::EnterRoundabout,
                                                    TurnType::EnterAndExitRoundabout,
                                                    TurnType::EnterRotary,
                                                    TurnType::EnterAndExitRotary,
                                                    TurnType::EnterRoundaboutIntersection,
                                                    TurnType::EnterAndExitRoundaboutIntersection,
                                                    TurnType::EnterRoundaboutAtExit,
                                                    TurnType::ExitRoundabout,
                                                    TurnType::EnterRotaryAtExit,
                                                    TurnType::ExitRotary,
                                                    TurnType::EnterRoundaboutIntersectionAtExit,
                                                    TurnType::ExitRoundaboutIntersection,
                                                    TurnType::StayOnRoundabout};

    const auto *first = valid_types;
    const auto *last = first + sizeof(valid_types) / sizeof(valid_types[0]);

    return std::find(first, last, instruction.type) != last;
}

inline bool entersRoundabout(const extractor::guidance::TurnInstruction instruction)
{
    return (instruction.type == extractor::guidance::TurnType::EnterRoundabout ||
            instruction.type == extractor::guidance::TurnType::EnterRotary ||
            instruction.type == extractor::guidance::TurnType::EnterRoundaboutIntersection ||
            instruction.type == extractor::guidance::TurnType::EnterRoundaboutAtExit ||
            instruction.type == extractor::guidance::TurnType::EnterRotaryAtExit ||
            instruction.type == extractor::guidance::TurnType::EnterRoundaboutIntersectionAtExit ||
            instruction.type == extractor::guidance::TurnType::EnterAndExitRoundabout ||
            instruction.type == extractor::guidance::TurnType::EnterAndExitRotary ||
            instruction.type == extractor::guidance::TurnType::EnterAndExitRoundaboutIntersection);
}

inline bool leavesRoundabout(const extractor::guidance::TurnInstruction instruction)
{
    return (instruction.type == extractor::guidance::TurnType::ExitRoundabout ||
            instruction.type == extractor::guidance::TurnType::ExitRotary ||
            instruction.type == extractor::guidance::TurnType::ExitRoundaboutIntersection ||
            instruction.type == extractor::guidance::TurnType::EnterAndExitRoundabout ||
            instruction.type == extractor::guidance::TurnType::EnterAndExitRotary ||
            instruction.type == extractor::guidance::TurnType::EnterAndExitRoundaboutIntersection);
}

inline bool staysOnRoundabout(const extractor::guidance::TurnInstruction instruction)
{
    return instruction.type == extractor::guidance::TurnType::StayOnRoundabout;
}

// Silent Turn Instructions are not to be mentioned to the outside world but
inline bool isSilent(const extractor::guidance::TurnInstruction instruction)
{
    return instruction.type == extractor::guidance::TurnType::NoTurn ||
           instruction.type == extractor::guidance::TurnType::Suppressed ||
           instruction.type == extractor::guidance::TurnType::StayOnRoundabout;
}

inline bool hasRampType(const extractor::guidance::TurnInstruction instruction)
{
    return instruction.type == extractor::guidance::TurnType::OffRamp ||
           instruction.type == extractor::guidance::TurnType::OnRamp;
}

inline extractor::guidance::DirectionModifier::Enum getTurnDirection(const double angle)
{
    // An angle of zero is a u-turn
    // 180 goes perfectly straight
    // 0-180 are right turns
    // 180-360 are left turns
    if (angle > 0 && angle < 60)
        return extractor::guidance::DirectionModifier::SharpRight;
    if (angle >= 60 && angle < 140)
        return extractor::guidance::DirectionModifier::Right;
    if (angle >= 140 && angle < 160)
        return extractor::guidance::DirectionModifier::SlightRight;
    if (angle >= 160 && angle <= 200)
        return extractor::guidance::DirectionModifier::Straight;
    if (angle > 200 && angle <= 220)
        return extractor::guidance::DirectionModifier::SlightLeft;
    if (angle > 220 && angle <= 300)
        return extractor::guidance::DirectionModifier::Left;
    if (angle > 300 && angle < 360)
        return extractor::guidance::DirectionModifier::SharpLeft;
    return extractor::guidance::DirectionModifier::UTurn;
}

// swaps left <-> right modifier types
OSRM_ATTR_WARN_UNUSED
inline extractor::guidance::DirectionModifier::Enum
mirrorDirectionModifier(const extractor::guidance::DirectionModifier::Enum modifier)
{
    const constexpr extractor::guidance::DirectionModifier::Enum results[] = {
        extractor::guidance::DirectionModifier::UTurn,
        extractor::guidance::DirectionModifier::SharpLeft,
        extractor::guidance::DirectionModifier::Left,
        extractor::guidance::DirectionModifier::SlightLeft,
        extractor::guidance::DirectionModifier::Straight,
        extractor::guidance::DirectionModifier::SlightRight,
        extractor::guidance::DirectionModifier::Right,
        extractor::guidance::DirectionModifier::SharpRight};
    return results[modifier];
}

inline bool hasLeftModifier(const extractor::guidance::TurnInstruction instruction)
{
    return instruction.direction_modifier == extractor::guidance::DirectionModifier::SharpLeft ||
           instruction.direction_modifier == extractor::guidance::DirectionModifier::Left ||
           instruction.direction_modifier == extractor::guidance::DirectionModifier::SlightLeft;
}

inline bool hasRightModifier(const extractor::guidance::TurnInstruction instruction)
{
    return instruction.direction_modifier == extractor::guidance::DirectionModifier::SharpRight ||
           instruction.direction_modifier == extractor::guidance::DirectionModifier::Right ||
           instruction.direction_modifier == extractor::guidance::DirectionModifier::SlightRight;
}

inline bool isLeftTurn(const extractor::guidance::TurnInstruction instruction)
{
    switch (instruction.type)
    {
    case TurnType::Merge:
        return hasRightModifier(instruction);
    default:
        return hasLeftModifier(instruction);
    }
}

inline bool isRightTurn(const extractor::guidance::TurnInstruction instruction)
{
    switch (instruction.type)
    {
    case TurnType::Merge:
        return hasLeftModifier(instruction);
    default:
        return hasRightModifier(instruction);
    }
}

inline DirectionModifier::Enum bearingToDirectionModifier(const double bearing)
{
    if (bearing < 135)
    {
        return extractor::guidance::DirectionModifier::Right;
    }

    if (bearing <= 225)
    {
        return extractor::guidance::DirectionModifier::Straight;
    }
    return extractor::guidance::DirectionModifier::Left;
}

} // namespace guidance
} // namespace extractor
} // namespace osrm

#endif // OSRM_GUIDANCE_TURN_INSTRUCTION_HPP_
