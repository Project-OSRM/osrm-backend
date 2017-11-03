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
    ExtractionTurn(double angle,
                   int number_of_roads,
                   bool is_u_turn,
                   bool has_traffic_light,
                   bool source_restricted,
                   bool target_restricted,
                   bool is_left_hand_driving,
                   TravelMode source_mode,
                   TravelMode target_mode)
        : angle(180. - angle), number_of_roads(number_of_roads), is_u_turn(is_u_turn),
          has_traffic_light(has_traffic_light), source_restricted(source_restricted),
          target_restricted(target_restricted), is_left_hand_driving(is_left_hand_driving),
          weight(0.), duration(0.), source_mode(source_mode), target_mode(target_mode)
    {
    }

    const double angle;
    const int number_of_roads;
    const bool is_u_turn;
    const bool has_traffic_light;
    const bool source_restricted;
    const bool target_restricted;
    const bool is_left_hand_driving;
    double weight;
    double duration;
    TravelMode source_mode;
    TravelMode target_mode;
};
}
}

#endif
