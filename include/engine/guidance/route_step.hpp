#ifndef ROUTE_STEP_HPP
#define ROUTE_STEP_HPP

#include "extractor/travel_mode.hpp"
#include "engine/guidance/step_maneuver.hpp"

#include <string>
#include <vector>

namespace osrm
{
namespace engine
{
namespace guidance
{
// Given the following turn from a,b to b,c over b:
//  a --> b --> c
// this struct saves the information of the segment b,c.
// Notable exceptions are Departure and Arrival steps.
// Departue: s --> a --> b. Represents the segment s,a with location being s.
// Arrive: a --> b --> t. The segment (b,t) is already covered by the previous segment.
struct RouteStep
{
    unsigned name_id;
    std::string name;
    double duration;
    double distance;
    extractor::TravelMode mode;
    StepManeuver maneuver;
    // indices into the locations array stored the LegGeometry
    std::size_t geometry_begin;
    std::size_t geometry_end;
};
}
}
}

#endif
