#include <cstddef>

#include "guidance/translator.hpp"
#include "guidance/guidance_toolkit.hpp"

#include "util/simple_logger.hpp"

namespace osrm
{
namespace guidance
{

namespace detail
{

// TODO decide on additional information needed, pass approriate sources
inline std::string translateContinue(const DirectionModifier modifier, const std::string &name)
{
    const constexpr char* base[detail::num_direction_modifiers] = {
        "Turn around and", "Turn sharp to the right to", "Turn right to",
        "Keep right to",   "Continue straight to",       "Keep left to",
        "Turn left to",    "Turn sharp left to"};
    return std::string(base[static_cast<int>(modifier)]) + " stay on " +
           ((name == "") ? "the current road." : name);
}

inline std::string translateTurn(const DirectionModifier modifier, const std::string &name)
{
    return modifier_names[static_cast<int>(modifier)] + (("" == name) ? "" : (" onto " + name));
}

inline std::string translateMerge(const std::string &name)
{
  return std::string("Merge") + (("" == name) ? "" : (" onto " + name));
}

inline std::string translateName(const DirectionModifier modifier, const std::string &name)
{
  const constexpr char* base[detail::num_direction_modifiers] = {
    "After a powerslide, the road name changes to ", "The road takes a sharp turn to the right and becomes ", "The road turns right and becomes ",
    "The road takes a slight turn to the right and becomes ", "The road goes straight into ", "The road takes a slight turn to the left and becomes ",
    "The road turns left and becomes ", "The road takes a sharp turn to the left and becomes " };

  return std::string(base[static_cast<int>(modifier)]) + ((name == "") ?  "an unnamed road" : name);
}

inline std::string translateRamp(const DirectionModifier modifier, const std::string &name)
{
    const constexpr char* base[detail::num_direction_modifiers] = {
      "You really fucked up to get here", "Make a sharp right and take the ramp",
      "Turn right and take the ramp", "Take the ramp", "Go straight onto the ramp",
      "Take the ramp on the left", "Turn left and take the ramp", "Make a sharp left and take the ramp"};

    std::string result = base[modifier];

    return result + (("" == name) ? "" : (" towards " + name));
}

inline std::string translateFork(const DirectionModifier modifier, const std::string &name)
{
    BOOST_ASSERT_MSG(isSlightLeftRight(modifier), "Fork modifier needs to be left/right");

    return std::string("At the fork, keep ") + ((modifier == DirectionModifier::SlightLeft) ? "left" : "right") +
           ((name == "") ? "" : (" and continue on " + name));
}

inline std::string translateEndOfRoad(const DirectionModifier modifier, const std::string &name)
{
    BOOST_ASSERT_MSG(isLeftRight(modifier), "End of Road modifier needs to be left/right");

    return std::string("At the end of the road, turn ") +
           ((modifier == DirectionModifier::Left) ? "left" : "right") +
           ((name == "") ? "" : (" onto " + name));
}

} // namespace detail

std::string
translateLocation(const LocationType type, const DirectionModifier modifier, const std::string &name)
{
    const constexpr char* compass_by_direction[] = {"South", "South East", "East", "North East",
                                              "North", "North West", "West", "South West"};
    std::string translation;
    switch (type)
    {
    case LocationType::Start:
        if (name != "")
        {
            return std::string("Head ") + compass_by_direction[static_cast<int>(modifier)] + " on " + name;
        }
        else
        {
            return std::string("Head ") + compass_by_direction[static_cast<int>(modifier)];
        }
        break;
    case LocationType::Intermediate:
        if (name != "")
            translation = "You have reached your intermediate destination (" + name + ").";
        else
            translation = "You have reached your intermediate destination.";
        break;
    case LocationType::Destination:
        if (name != "")
            translation = "You have reached your destination (" + name + ").";
        else
            translation = "You have reached your destination.";
        break;
    }
    if (modifier == DirectionModifier::Straight )
        translation += " Your destination is ahead of you.";
    if (modifier == DirectionModifier::Right)
        translation += " Your destination is on the right.";
    else if (modifier == DirectionModifier::Left)
        translation += " Your destination is on the left.";

    return translation;
}


std::string translate(const TurnInstruction instruction, const std::string &name)
{
    switch (instruction.type)
    {
    case TurnType::Continue:
        return detail::translateContinue(instruction.direction_modifier, name);
    case TurnType::Turn:
        return detail::translateTurn(instruction.direction_modifier, name);
    case TurnType::Ramp:
        return detail::translateRamp(instruction.direction_modifier, name);
    case TurnType::Merge:
        return detail::translateMerge(name);
    case TurnType::NewName:
        return detail::translateName(instruction.direction_modifier,name);
    case TurnType::Fork:
        return detail::translateFork(instruction.direction_modifier, name);
    case TurnType::EndOfRoad:
        return detail::translateEndOfRoad(instruction.direction_modifier, name);
    default:
        return "Instruction (type: " + std::to_string(static_cast<int>(instruction.type)) +
               ", direction_modifier: " + std::to_string(static_cast<int>(instruction.direction_modifier)) +
               ") not handled";
    }
}

std::string translateRoundabout( const LocationType location, const std::string &name, int exit )
{
  if( location == LocationType::Start )
  {
      return "Enter the roundabout" + std::to_string(exit) + ((name != "") ? (" (" + name + ")") : "");
  }
  return std::string("Take exit Nr ") + std::to_string(exit) + ((name != "") ? (" onto " + name) : "");
}

} // namespace guidance
} // namespace osrm
