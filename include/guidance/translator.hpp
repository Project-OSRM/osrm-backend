#ifndef OSRM_GUIDANCE_TRANSLATOR_HPP_
#define OSRM_GUIDANCE_TRANSLATOR_HPP_

#include <string>

#include "guidance/turn_instruction.hpp"

namespace osrm
{
namespace guidance
{

// TODO decide on additional information needed, pass approriate sources
std::string
translateLocation(const LocationType type, const DirectionModifier modifier, const std::string &name);
std::string
translateRoundabout(const LocationType type, const std::string &name, const int exit_nr);
std::string translate(const TurnInstruction instruction, const std::string &name);

} // namespace guidance
} // namespace osrm

#endif // OSRM_GUIDANCE_TRANSLATOR_HPP_
