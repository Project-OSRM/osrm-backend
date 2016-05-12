#ifndef OSRM_EXTRACTION_TURN_HPP
#define OSRM_EXTRACTION_TURN_HPP

#include <boost/numeric/conversion/cast.hpp>

#include <extractor/guidance/turn_instruction.hpp>

#include <cstdint>

namespace osrm
{
namespace extractor
{

struct ExtractionTurn
{
    ExtractionTurn(const double angle_, const guidance::TurnType::Enum turn_type_, const guidance::DirectionModifier::Enum direction_modifier_)
        : angle(angle_), turn_type(turn_type_), direction_modifier(direction_modifier_), duration(0.), weight(0.)
    {
    }

    const double angle;
    const guidance::TurnType::Enum turn_type;
    const guidance::DirectionModifier::Enum direction_modifier;
    double duration;
    double weight;
};
}
}

#endif
