#ifndef OSRM_EXTRACTION_TURN_HPP
#define OSRM_EXTRACTION_TURN_HPP

#include <boost/numeric/conversion/cast.hpp>

#include <extractor/guidance/intersection.hpp>

#include <cstdint>

namespace osrm
{
namespace extractor
{

struct ExtractionTurn
{
    ExtractionTurn(const guidance::ConnectedRoad &turn,
                   bool has_traffic_light,
                   bool source_restricted,
                   bool target_restricted,
                   bool is_left_hand_driving)
        : angle(180. - turn.angle), turn_type(turn.instruction.type),
          direction_modifier(turn.instruction.direction_modifier),
          has_traffic_light(has_traffic_light), source_restricted(source_restricted),
          target_restricted(target_restricted), is_left_hand_driving(is_left_hand_driving),
          weight(0.), duration(0.)
    {
    }

    ExtractionTurn(bool has_traffic_light,
                   bool source_restricted,
                   bool target_restricted,
                   bool is_left_hand_driving)
        : angle(0), turn_type(guidance::TurnType::NoTurn),
          direction_modifier(guidance::DirectionModifier::Straight),
          has_traffic_light(has_traffic_light), source_restricted(source_restricted),
          target_restricted(target_restricted), is_left_hand_driving(is_left_hand_driving),
          weight(0.), duration(0.)
    {
    }

    const double angle;
    const guidance::TurnType::Enum turn_type;
    const guidance::DirectionModifier::Enum direction_modifier;
    const bool has_traffic_light;
    const bool source_restricted;
    const bool target_restricted;
    const bool is_left_hand_driving;

    double weight;
    double duration;
};
}
}

#endif
