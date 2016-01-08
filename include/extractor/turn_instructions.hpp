#ifndef TURN_INSTRUCTIONS_HPP
#define TURN_INSTRUCTIONS_HPP

namespace osrm
{
namespace extractor
{

enum class TurnInstruction : unsigned char
{
    NoTurn = 0,
    GoStraight,
    TurnSlightRight,
    TurnRight,
    TurnSharpRight,
    UTurn,
    TurnSharpLeft,
    TurnLeft,
    TurnSlightLeft,
    ReachViaLocation,
    HeadOn,
    EnterRoundAbout,
    LeaveRoundAbout,
    StayOnRoundAbout,
    StartAtEndOfStreet,
    ReachedYourDestination,
    EnterAgainstAllowedDirection,
    LeaveAgainstAllowedDirection,
    InverseAccessRestrictionFlag = 127,
    AccessRestrictionFlag = 128,
    AccessRestrictionPenalty = 129
};

// Translates between angles and their human-friendly directional representation
inline TurnInstruction getTurnDirection(const double angle)
{
    if (angle >= 23 && angle < 67)
    {
        return TurnInstruction::TurnSharpRight;
    }
    if (angle >= 67 && angle < 113)
    {
        return TurnInstruction::TurnRight;
    }
    if (angle >= 113 && angle < 158)
    {
        return TurnInstruction::TurnSlightRight;
    }
    if (angle >= 158 && angle < 202)
    {
        return TurnInstruction::GoStraight;
    }
    if (angle >= 202 && angle < 248)
    {
        return TurnInstruction::TurnSlightLeft;
    }
    if (angle >= 248 && angle < 292)
    {
        return TurnInstruction::TurnLeft;
    }
    if (angle >= 292 && angle < 336)
    {
        return TurnInstruction::TurnSharpLeft;
    }
    return TurnInstruction::UTurn;
}

// Decides if a turn is needed to be done for the current instruction
inline bool isTurnNecessary(const TurnInstruction turn_instruction)
{
    if (TurnInstruction::NoTurn == turn_instruction ||
        TurnInstruction::StayOnRoundAbout == turn_instruction)
    {
        return false;
    }
    return true;
}
}
}

#endif /* TURN_INSTRUCTIONS_HPP */
