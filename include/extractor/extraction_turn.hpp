#ifndef OSRM_EXTRACTION_TURN_HPP
#define OSRM_EXTRACTION_TURN_HPP

#include <boost/numeric/conversion/cast.hpp>

#include <extractor/guidance/intersection.hpp>

#include <cstdint>

namespace osrm {
namespace extractor {

struct ExtractionTurnLeg {
  ExtractionTurnLeg(bool is_restricted, TravelMode mode, bool is_motorway,
                    bool is_link, int number_of_lanes)
      : is_restricted(is_restricted),
        mode(mode),
        is_motorway(is_motorway),
        is_link(is_link),
        number_of_lanes(number_of_lanes) {}

  const bool is_restricted;
  const TravelMode mode;
  const bool is_motorway;
  const bool is_link;
  const int number_of_lanes;
};

struct ExtractionTurn {
  ExtractionTurn(double angle, int number_of_roads, bool is_u_turn,
                 bool has_traffic_light, bool is_left_hand_driving)
      : angle(180. - angle),
        number_of_roads(number_of_roads),
        is_u_turn(is_u_turn),
        has_traffic_light(has_traffic_light),
        is_left_hand_driving(is_left_hand_driving),
        weight(0.),
        duration(0.),

        // depr aaarghARGH @CHAUTODO
        source_mode(TRAVEL_MODE_DRIVING),
        target_mode(TRAVEL_MODE_DRIVING),
        source_restricted(true),
        target_restricted(true) {}

  const double angle;
  const int number_of_roads;
  const bool is_u_turn;
  const bool has_traffic_light;
  const bool is_left_hand_driving;

  // std::vector<ExtractionTurnLeg> roads_on_the_right;
  // std::vector<ExtractionTurnLeg> roads_on_the_left;

  double weight;
  double duration;



  // deprecated ARGH @CHAUTODO
  const TravelMode source_mode;
  const TravelMode target_mode;
  const bool source_restricted;
  const bool target_restricted;
};
}
}

#endif
