#ifndef OSRM_EXTRACTION_TURN_HPP
#define OSRM_EXTRACTION_TURN_HPP

#include "extractor/graph_compressor.hpp"
#include "extractor/road_classification.hpp"
#include "extractor/travel_mode.hpp"
#include "util/typedefs.hpp"

#include <boost/numeric/conversion/cast.hpp>

#include <vector>

namespace osrm::extractor
{

struct ExtractionTurnLeg
{
    using EdgeData = util::NodeBasedDynamicGraph::EdgeData;

    ExtractionTurnLeg(bool is_restricted,
                      bool is_motorway,
                      bool is_link,
                      int number_of_lanes,
                      int highway_turn_classification,
                      int access_turn_classification,
                      int speed,
                      double distance,
                      RoadPriorityClass::Enum priority_class,
                      bool is_incoming,
                      bool is_outgoing)
        : is_restricted(is_restricted), is_motorway(is_motorway), is_link(is_link),
          number_of_lanes(number_of_lanes),
          highway_turn_classification(highway_turn_classification),
          access_turn_classification(access_turn_classification), speed(speed), distance(distance),
          priority_class(priority_class), is_incoming(is_incoming), is_outgoing(is_outgoing)
    {
    }

    ExtractionTurnLeg(EdgeData edge_data, bool is_incoming, bool is_outgoing)
        : ExtractionTurnLeg(edge_data.flags.restricted,
                            edge_data.flags.road_classification.IsMotorwayClass(),
                            edge_data.flags.road_classification.IsLinkClass(),
                            edge_data.flags.road_classification.GetNumberOfLanes(),
                            edge_data.flags.highway_turn_classification,
                            edge_data.flags.access_turn_classification,
                            36.0 * from_alias<double>(edge_data.distance) /
                                from_alias<double>(edge_data.duration),
                            from_alias<double>(edge_data.distance),
                            edge_data.flags.road_classification.GetPriority(),
                            is_incoming,
                            is_outgoing)
    {
    }

    bool is_restricted;
    bool is_motorway;
    bool is_link;
    int number_of_lanes;
    int highway_turn_classification;
    int access_turn_classification;
    int speed;
    double distance;
    RoadPriorityClass::Enum priority_class;
    bool is_incoming;
    bool is_outgoing;
};

// Structure reflected into LUA scripting
//
// This structure backs the `turn` parameter in the LUA `process_turn` function.
// That function is called:
//
// 1. from graph_compressor.cpp when compressing nodes with obstacles.  A fake turn with
//    just one incoming and one outgoing edge will be generated. `number_of_roads` will
//    be 2. The returned penalties will be added to the compressed edge.
//
// 2. from edge_based_graph_factory.cpp during generation of edge-expanded edges. In
//    this stage real turns will be modelled. The returned penalties will be added to
//    the edge-expanded edge.

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

                   const ExtractionTurnLeg &source_road,
                   const ExtractionTurnLeg &target_road,
                   const std::vector<ExtractionTurnLeg> &roads_on_the_right,
                   const std::vector<ExtractionTurnLeg> &roads_on_the_left,
                   const NodeID from,
                   const NodeID via,
                   const NodeID to)
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

          from(from), via(via), to(to),

          source_road(source_road), target_road(target_road),
          roads_on_the_right(roads_on_the_right), roads_on_the_left(roads_on_the_left),

          weight(0.), duration(0.)
    {
    }

    // to construct a "fake" turn while compressing obstacle nodes
    // in graph_compressor.cpp
    ExtractionTurn(NodeID from,
                   NodeID via,
                   NodeID to,
                   const ExtractionTurnLeg::EdgeData &source_edge,
                   const ExtractionTurnLeg::EdgeData &target_edge,
                   const std::vector<ExtractionTurnLeg> &roads_on_the_right,
                   const std::vector<ExtractionTurnLeg> &roads_on_the_left)
        : ExtractionTurn{0,
                         2,
                         false,
                         true,
                         false,
                         // source
                         false,
                         TRAVEL_MODE_DRIVING,
                         false,
                         false,
                         1,
                         0,
                         0,
                         0,
                         0,
                         // target
                         false,
                         TRAVEL_MODE_DRIVING,
                         false,
                         false,
                         1,
                         0,
                         0,
                         0,
                         0,
                         // other
                         ExtractionTurnLeg{source_edge, true, false},
                         ExtractionTurnLeg{target_edge, false, true},
                         roads_on_the_right,
                         roads_on_the_left,
                         from,
                         via,
                         to}
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

    const NodeID from;
    const NodeID via;
    const NodeID to;

    const ExtractionTurnLeg source_road;
    const ExtractionTurnLeg target_road;
    const std::vector<ExtractionTurnLeg> roads_on_the_right;
    const std::vector<ExtractionTurnLeg> roads_on_the_left;

    double weight;
    double duration;
};
} // namespace osrm::extractor

#endif
