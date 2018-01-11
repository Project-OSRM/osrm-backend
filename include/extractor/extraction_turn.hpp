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
                 bool has_traffic_light, bool is_left_hand_driving,
                 bool source_restricted, TravelMode source_mode,
                 bool source_is_motorway, bool source_is_link,
                 int source_number_of_lanes, bool target_restricted,
                 TravelMode target_mode, bool target_is_motorway,
                 bool target_is_link, int target_number_of_lanes,
                 std::vector<ExtractionTurnLeg> &roads_on_the_right,
                 std::vector<ExtractionTurnLeg> &roads_on_the_left)
      : angle(180. - angle),
        number_of_roads(number_of_roads),
        is_u_turn(is_u_turn),
        has_traffic_light(has_traffic_light),
        is_left_hand_driving(is_left_hand_driving),

        source_restricted(source_restricted),
        source_mode(source_mode),
        source_is_motorway(source_is_motorway),
        source_is_link(source_is_link),
        source_number_of_lanes(source_number_of_lanes),

        target_restricted(target_restricted),
        target_mode(target_mode),
        target_is_motorway(target_is_motorway),
        target_is_link(target_is_link),
        target_number_of_lanes(target_number_of_lanes),

        roads_on_the_right(roads_on_the_right),
        roads_on_the_left(roads_on_the_left),
        weight(0.),
        duration(0.)

  {
    BOOST_ASSERT_MSG(
        !is_u_turn || roads_on_the_left.size() == 0,
        "there cannot be roads on the left when there is a u turn");
    BOOST_ASSERT_MSG(roads_on_the_right.size() + roads_on_the_left.size() + is_u_turn + 2 ==
                         number_of_roads,
                     "number of roads at intersection do not match");
  }
  const double angle;
  const int number_of_roads;
  const bool is_u_turn;
  const bool has_traffic_light;
  const bool is_left_hand_driving;

  // source info
  const bool source_restricted;
  const TravelMode source_mode;
  const bool source_is_motorway;
  const bool source_is_link;
  const int source_number_of_lanes;

  // target info
  const bool target_restricted;
  const TravelMode target_mode;
  const bool target_is_motorway;
  const bool target_is_link;
  const int target_number_of_lanes;

  const std::vector<ExtractionTurnLeg> roads_on_the_right;
  const std::vector<ExtractionTurnLeg> roads_on_the_left;

  double weight;
  double duration;
};
}
}

#endif
