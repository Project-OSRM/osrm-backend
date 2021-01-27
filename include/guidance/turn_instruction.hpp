#ifndef OSRM_GUIDANCE_TURN_INSTRUCTION_HPP_
#define OSRM_GUIDANCE_TURN_INSTRUCTION_HPP_

#include <algorithm>
#include <cstdint>

#include "guidance/roundabout_type.hpp"
#include "util/attributes.hpp"
#include "util/typedefs.hpp"

namespace osrm
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
} // namespace DirectionModifier

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
// depreacted: const constexpr Enum UseLane = 16; // No Turn, but you need to stay on a given lane!

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
} // namespace TurnType

struct TurnInstruction
{
    TurnInstruction(const TurnType::Enum type = TurnType::Invalid,
                    const DirectionModifier::Enum direction_modifier = DirectionModifier::UTurn)
        : type(type), direction_modifier(direction_modifier)
    {
    }

    TurnType::Enum type : 5;
    DirectionModifier::Enum direction_modifier : 3;

    bool IsUTurn() const
    {
        return type != TurnType::NoTurn && direction_modifier == DirectionModifier::UTurn;
    }

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
    using namespace guidance::TurnType;
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

inline bool entersRoundabout(const guidance::TurnInstruction instruction)
{
    return (instruction.type == guidance::TurnType::EnterRoundabout ||
            instruction.type == guidance::TurnType::EnterRotary ||
            instruction.type == guidance::TurnType::EnterRoundaboutIntersection ||
            instruction.type == guidance::TurnType::EnterRoundaboutAtExit ||
            instruction.type == guidance::TurnType::EnterRotaryAtExit ||
            instruction.type == guidance::TurnType::EnterRoundaboutIntersectionAtExit ||
            instruction.type == guidance::TurnType::EnterAndExitRoundabout ||
            instruction.type == guidance::TurnType::EnterAndExitRotary ||
            instruction.type == guidance::TurnType::EnterAndExitRoundaboutIntersection);
}

inline bool leavesRoundabout(const guidance::TurnInstruction instruction)
{
    return (instruction.type == guidance::TurnType::ExitRoundabout ||
            instruction.type == guidance::TurnType::ExitRotary ||
            instruction.type == guidance::TurnType::ExitRoundaboutIntersection ||
            instruction.type == guidance::TurnType::EnterAndExitRoundabout ||
            instruction.type == guidance::TurnType::EnterAndExitRotary ||
            instruction.type == guidance::TurnType::EnterAndExitRoundaboutIntersection);
}

inline bool staysOnRoundabout(const guidance::TurnInstruction instruction)
{
    return instruction.type == guidance::TurnType::StayOnRoundabout ||
           instruction.type == guidance::TurnType::EnterRoundaboutAtExit ||
           instruction.type == guidance::TurnType::EnterRotaryAtExit ||
           instruction.type == guidance::TurnType::EnterRoundaboutIntersectionAtExit;
}

// Silent Turn Instructions are not to be mentioned to the outside world but
inline bool isSilent(const guidance::TurnInstruction instruction)
{
    return instruction.type == guidance::TurnType::NoTurn ||
           instruction.type == guidance::TurnType::Suppressed ||
           instruction.type == guidance::TurnType::StayOnRoundabout;
}

inline bool hasRampType(const guidance::TurnInstruction instruction)
{
    return instruction.type == guidance::TurnType::OffRamp ||
           instruction.type == guidance::TurnType::OnRamp;
}

inline guidance::DirectionModifier::Enum getTurnDirection(const double angle)
{
    // An angle of zero is a u-turn
    // 180 goes perfectly straight
    // 0-180 are right turns
    // 180-360 are left turns
    if (angle > 0 && angle < 60)
        return guidance::DirectionModifier::SharpRight;
    if (angle >= 60 && angle < 140)
        return guidance::DirectionModifier::Right;
    if (angle >= 140 && angle < 160)
        return guidance::DirectionModifier::SlightRight;
    if (angle >= 160 && angle <= 200)
        return guidance::DirectionModifier::Straight;
    if (angle > 200 && angle <= 220)
        return guidance::DirectionModifier::SlightLeft;
    if (angle > 220 && angle <= 300)
        return guidance::DirectionModifier::Left;
    if (angle > 300 && angle < 360)
        return guidance::DirectionModifier::SharpLeft;
    return guidance::DirectionModifier::UTurn;
}

// swaps left <-> right modifier types
OSRM_ATTR_WARN_UNUSED
inline guidance::DirectionModifier::Enum
mirrorDirectionModifier(const guidance::DirectionModifier::Enum modifier)
{
    const constexpr guidance::DirectionModifier::Enum results[] = {
        guidance::DirectionModifier::UTurn,
        guidance::DirectionModifier::SharpLeft,
        guidance::DirectionModifier::Left,
        guidance::DirectionModifier::SlightLeft,
        guidance::DirectionModifier::Straight,
        guidance::DirectionModifier::SlightRight,
        guidance::DirectionModifier::Right,
        guidance::DirectionModifier::SharpRight};
    return results[modifier];
}

inline bool hasLeftModifier(const guidance::TurnInstruction instruction)
{
    return instruction.direction_modifier == guidance::DirectionModifier::SharpLeft ||
           instruction.direction_modifier == guidance::DirectionModifier::Left ||
           instruction.direction_modifier == guidance::DirectionModifier::SlightLeft;
}

inline bool hasRightModifier(const guidance::TurnInstruction instruction)
{
    return instruction.direction_modifier == guidance::DirectionModifier::SharpRight ||
           instruction.direction_modifier == guidance::DirectionModifier::Right ||
           instruction.direction_modifier == guidance::DirectionModifier::SlightRight;
}

inline bool isLeftTurn(const guidance::TurnInstruction instruction)
{
    switch (instruction.type)
    {
    case TurnType::Merge:
        return hasRightModifier(instruction);
    default:
        return hasLeftModifier(instruction);
    }
}

inline bool isRightTurn(const guidance::TurnInstruction instruction)
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
        return guidance::DirectionModifier::Right;
    }

    if (bearing <= 225)
    {
        return guidance::DirectionModifier::Straight;
    }
    return guidance::DirectionModifier::Left;
}

namespace detail
{

const constexpr char *modifier_names[] = {"uturn",
                                          "sharp right",
                                          "right",
                                          "slight right",
                                          "straight",
                                          "slight left",
                                          "left",
                                          "sharp left",
                                          "UNDEFINED"};

/**
 * Human readable values for TurnType enum values
 */
struct TurnTypeName
{
    // String value we return with our API
    const char *external_name;
    // Internal only string name for the turn type - useful for debugging
    // and used by debug tiles for visualizing hidden turn types
    const char *internal_name;
};

// Indexes in this list correspond to the Enum values of osrm::guidance::TurnType
const constexpr TurnTypeName turn_type_names[] = {
    {"invalid", "(not set)"},
    {"new name", "new name"},
    {"continue", "continue"},
    {"turn", "turn"},
    {"merge", "merge"},
    {"on ramp", "on ramp"},
    {"off ramp", "off ramp"},
    {"fork", "fork"},
    {"end of road", "end of road"},
    {"notification", "notification"},
    {"roundabout", "enter roundabout"},
    {"exit roundabout", "enter and exit roundabout"},
    {"rotary", "enter rotary"},
    {"exit rotary", "enter and exit rotary"},
    {"roundabout turn", "enter roundabout turn"},
    {"roundabout turn", "enter and exit roundabout turn"},
    {"use lane", "use lane"},
    {"invalid", "(noturn)"},
    {"invalid", "(suppressed)"},
    {"roundabout", "roundabout"},
    {"exit roundabout", "exit roundabout"},
    {"rotary", "rotary"},
    {"exit rotary", "exit rotary"},
    {"roundabout turn", "roundabout turn"},
    {"exit roundabout", "exit roundabout turn"},
    {"invalid", "(stay on roundabout)"},
    {"invalid", "(sliproad)"},
    {"MAXVALUE", "MAXVALUE"}};

} // namespace detail

inline std::string instructionTypeToString(const TurnType::Enum type)
{
    static_assert((sizeof(detail::turn_type_names) + 1) / sizeof(detail::turn_type_names[0]) >=
                      TurnType::MaxTurnType,
                  "Some turn types have no string representation.");
    return detail::turn_type_names[static_cast<std::size_t>(type)].external_name;
}

inline std::string internalInstructionTypeToString(const TurnType::Enum type)
{
    static_assert((sizeof(detail::turn_type_names) + 1) / sizeof(detail::turn_type_names[0]) >=
                      TurnType::MaxTurnType,
                  "Some turn types have no string representation.");
    return detail::turn_type_names[static_cast<std::size_t>(type)].internal_name;
}

inline std::string instructionModifierToString(const DirectionModifier::Enum modifier)
{
    static_assert((sizeof(detail::modifier_names) + 1) / sizeof(detail::modifier_names[0]) >=
                      DirectionModifier::MaxDirectionModifier,
                  "Some direction modifiers have no string representation.");
    return detail::modifier_names[static_cast<std::size_t>(modifier)];
}

} // namespace guidance
} // namespace osrm

#endif // OSRM_GUIDANCE_TURN_INSTRUCTION_HPP_
