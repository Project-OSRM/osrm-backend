#ifndef OSRM_GUIDANCE_TURN_INSTRUCTION_HPP_
#define OSRM_GUIDANCE_TURN_INSTRUCTION_HPP_

#include <cstdint>

#include <boost/assert.hpp>

namespace osrm
{
namespace engine
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

const constexpr char *modifier_names[detail::num_direction_modifiers] = {
    "uturn",    "sharp right", "right", "slight right",
    "straight", "slight left", "left",  "sharp left"};

enum LocationType
{
    Start,
    Intermediate,
    Destination
};

// enum class TurnType : unsigned char
enum TurnType // at the moment we can support 32 turn types, without increasing memory consumption
{
    Invalid,                // no valid turn instruction
    NoTurn,                 // end of segment without turn
    Location,               // start,end,via
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
    ExitRoundabout,         // Exiting a small Roundabout
    EnterRotary,            // Enter a rotary
    EnterRotaryAtExit,      // Enter A Rotary at a countable exit
    ExitRotary,             // Exit a rotary
    StayOnRoundabout,       // Continue on Either a small or a large Roundabout
    Restriction,            // Cross a Barrier, requires barrier penalties instead of full block
    Notification            // Travel Mode Changes`
};

inline bool isValidModifier( const TurnType type, const DirectionModifier modifier )
{
  if( type == TurnType::Location && 
      modifier != DirectionModifier::Left
      && modifier != DirectionModifier::Straight
      && modifier != DirectionModifier::Right )
    return false;
  return true;
}

const constexpr char *turn_type_names[] = {"invalid",
                                           "no turn",
                                           "waypoint",
                                           "passing intersection",
                                           "new name",
                                           "continue",
                                           "turn",
                                           "merge",
                                           "ramp",
                                           "fork",
                                           "end of road",
                                           "roundabout",
                                           "invalid"
                                           "invalid",
                                           "traffic circle",
                                           "invalid",
                                           "invalid",
                                           "invalid",
                                           "restriction",
                                           "notification"};

// turn angle in 1.40625 degree -> 128 == 180 degree
typedef uint8_t DiscreteAngle;

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
        return TurnInstruction(TurnType::NoTurn, DirectionModifier::Straight);
    }

    static TurnInstruction REMAIN_ROUNDABOUT(const DirectionModifier modifier)
    {
        return TurnInstruction(TurnType::StayOnRoundabout, modifier);
    }

    static TurnInstruction ENTER_ROUNDABOUT(const DirectionModifier modifier)
    {
          return TurnInstruction(TurnType::EnterRoundabout, modifier);
    }

    static TurnInstruction EXIT_ROUNDABOUT(const DirectionModifier modifier)
    {
        return TurnInstruction(TurnType::ExitRoundabout, modifier);
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

// Silent Turn Instructions are not to be mentioned to the outside world
inline bool isSilent(const TurnInstruction instruction)
{
    return instruction.type == TurnType::NoTurn || instruction.type == TurnType::Suppressed ||
           instruction.type == TurnType::StayOnRoundabout;
}

} // namespace guidance
} // namespace engine
} // namespace osrm

#endif // OSRM_GUIDANCE_TURN_INSTRUCTION_HPP_
