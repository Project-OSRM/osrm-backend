#ifndef ROUTE_STEP_HPP
#define ROUTE_STEP_HPP

#include "engine/guidance/step_maneuver.hpp"
#include "extractor/travel_mode.hpp"
#include "util/coordinate.hpp"
#include "util/guidance/bearing_class.hpp"
#include "util/guidance/entry_class.hpp"

#include <cstddef>

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

// A represenetation of intermediate intersections
struct Intersection
{
    double duration;
    double distance;
    util::Coordinate location;
    double bearing_before;
    double bearing_after;
    util::guidance::EntryClass entry_class;
    util::guidance::BearingClass bearing_class;
};

inline Intersection getInvalidIntersection()
{
    return {0,
            0,
            util::Coordinate{util::FloatLongitude{0.0}, util::FloatLatitude{0.0}},
            0,
            0,
            util::guidance::EntryClass(),
            util::guidance::BearingClass()};
}

struct RouteStep
{
    unsigned name_id;
    std::string name;
    std::string rotary_name;
    double duration;
    double distance;
    extractor::TravelMode mode;
    StepManeuver maneuver;
    // indices into the locations array stored the LegGeometry
    std::size_t geometry_begin;
    std::size_t geometry_end;
    std::vector<Intersection> intersections;
};

inline RouteStep getInvalidRouteStep()
{
    return {0,
            "",
            "",
            0,
            0,
            TRAVEL_MODE_INACCESSIBLE,
            getInvalidStepManeuver(),
            0,
            0,
            std::vector<Intersection>(1, getInvalidIntersection())};
}
}
}
}

#endif
