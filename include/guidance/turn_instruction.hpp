#ifndef OSRM_GUIDANCE_TURN_INSTRUCTION_HPP_
#define OSRM_GUIDANCE_TURN_INSTRUCTION_HPP_

#include <cstdint>

#include <boost/assert.hpp>

namespace osrm
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
    "Turn around", "Turn sharp right", "Turn right", "Turn slight right",
    "Go straight", "Turn slight left", "Turn left",  "Turn sharp left"};

enum LocationType
{
  Start,
  Intermediate,
  Destination
};

// enum class TurnType : unsigned char
enum TurnType // at the moment we can support 32 turn types, without increasing memory consumption
{
    Invalid,          // no valid turn instruction
    NoTurn,           // end of segment without turn
    Suppressed,       // location that suppresses a turn
    Location,         // start,end,via
    NewName,          // no turn, but name changes
    Turn,             // basic turn
    Merge,            // merge onto a street
    Ramp,             // special turn (highway ramp exits)
    Fork,             // fork road splitting up
    EndOfRoad,        // T intersection
    EnterRoundabout,  // Enetering a small Roundabout
    ExitRoundabout,   // Exiting a small Roundabout
    EnterRotary,      // Enter a rotary
    ExitRotary,       // Exit a rotary
    StayOnRoundabout, // Continue on Either a small or a large Roundabout
    Continue,         // remain on a street
    Restriction,      // Cross a Barrier, requires barrier penalties instead of full block
    Notification      // Travel Mode Changes
};

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
      return TurnInstruction(TurnType::Invalid,DirectionModifier::UTurn);
    }

    static TurnInstruction NO_TURN()
    {
      return TurnInstruction(TurnType::NoTurn,DirectionModifier::Straight);
    }

    static TurnInstruction REMAIN_ROUNDABOUT(const DirectionModifier modifier)
    {
      return TurnInstruction(TurnType::StayOnRoundabout,modifier);
    }

    static TurnInstruction ENTER_ROUNDABOUT(const DirectionModifier modifier)
    {
      return TurnInstruction(TurnType::EnterRoundabout,modifier);
    }

    static TurnInstruction EXIT_ROUNDABOUT(const DirectionModifier modifier)
    {
      return TurnInstruction(TurnType::ExitRoundabout,modifier);
    }

};

} // namespace guidance
} // namespace osrm

#endif // OSRM_GUIDANCE_TURN_INSTRUCTION_HPP_
