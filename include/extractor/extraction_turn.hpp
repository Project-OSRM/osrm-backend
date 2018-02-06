#ifndef OSRM_EXTRACTION_TURN_HPP
#define OSRM_EXTRACTION_TURN_HPP

#include "extractor/road_classification.hpp"
#include "extractor/travel_mode.hpp"

#include <boost/numeric/conversion/cast.hpp>

#include <cstdint>

namespace osrm
{
namespace extractor
{

struct ExtractionTurnLeg
{
    ExtractionTurnLeg(bool is_restricted,
                      bool is_motorway,
                      bool is_link,
                      int number_of_lanes,
                      int highway_turn_classification,
                      int access_turn_classification,
                      int speed,
                      RoadPriorityClass::Enum priority_class,
                      bool is_incoming,
                      bool is_outgoing)
        : is_restricted(is_restricted), is_motorway(is_motorway), is_link(is_link),
          number_of_lanes(number_of_lanes),
          highway_turn_classification(highway_turn_classification),
          access_turn_classification(access_turn_classification), speed(speed),
          priority_class(priority_class), is_incoming(is_incoming), is_outgoing(is_outgoing)
    {
    }

    const bool is_restricted;
    const bool is_motorway;
    const bool is_link;
    const int number_of_lanes;
    const int highway_turn_classification;
    const int access_turn_classification;
    const int speed;
    const RoadPriorityClass::Enum priority_class;
    const bool is_incoming;
    const bool is_outgoing;
};

struct ExtractionTurn
{
    ExtractionTurn(double angle,
                   int number_of_roads,
                   bool is_u_turn,
                   bool has_traffic_light,
                   bool is_left_hand_driving,
                   bool source_restricted,
                   TravelMode source_mode,
                   bool source_is_motorway,
                   bool source_is_link,
                   int source_number_of_lanes,
                   int source_highway_turn_classification,
                   int source_access_turn_classification,
                   int source_speed,
                   RoadPriorityClass::Enum source_priority_class,

                   bool target_restricted,
                   TravelMode target_mode,
                   bool target_is_motorway,
                   bool target_is_link,
                   int target_number_of_lanes,
                   int target_highway_turn_classification,
                   int target_access_turn_classification,
                   int target_speed,
                   RoadPriorityClass::Enum target_priority_class,

                   const std::vector<ExtractionTurnLeg> &roads_on_the_right,
                   const std::vector<ExtractionTurnLeg> &roads_on_the_left)
        : angle(180. - angle), number_of_roads(number_of_roads), is_u_turn(is_u_turn),
          has_traffic_light(has_traffic_light), is_left_hand_driving(is_left_hand_driving),

          source_restricted(source_restricted), source_mode(source_mode),
          source_is_motorway(source_is_motorway), source_is_link(source_is_link),
          source_number_of_lanes(source_number_of_lanes),
          source_highway_turn_classification(source_highway_turn_classification),
          source_access_turn_classification(source_access_turn_classification),
          source_speed(source_speed), source_priority_class(source_priority_class),

          target_restricted(target_restricted), target_mode(target_mode),
          target_is_motorway(target_is_motorway), target_is_link(target_is_link),
          target_number_of_lanes(target_number_of_lanes),
          target_highway_turn_classification(target_highway_turn_classification),
          target_access_turn_classification(target_access_turn_classification),
          target_speed(target_speed), target_priority_class(target_priority_class),

          roads_on_the_right(roads_on_the_right), roads_on_the_left(roads_on_the_left), weight(0.),
          duration(0.)

    {
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
    const int source_highway_turn_classification;
    const int source_access_turn_classification;
    const int source_speed;
    const RoadPriorityClass::Enum source_priority_class;

    // target info
    const bool target_restricted;
    const TravelMode target_mode;
    const bool target_is_motorway;
    const bool target_is_link;
    const int target_number_of_lanes;
    const int target_highway_turn_classification;
    const int target_access_turn_classification;
    const int target_speed;
    const RoadPriorityClass::Enum target_priority_class;

    const std::vector<ExtractionTurnLeg> roads_on_the_right;
    const std::vector<ExtractionTurnLeg> roads_on_the_left;

    double weight;
    double duration;
};
}
}

#endif
