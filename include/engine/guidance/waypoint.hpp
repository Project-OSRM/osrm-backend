#ifndef OSRM_ENGINE_GUIDANCE_WAYPOINT_HPP_
#define OSRM_ENGINE_GUIDANCE_WAYPOINT_HPP_

#include "engine/hint.hpp"
#include "util/coordinate.hpp"

#include <string>

namespace osrm
{
namespace engine
{
namespace guidance
{

struct Waypoint
{
    util::Coordinate location;
    std::string name;
    Hint hint;
};

}
}
}

#endif
