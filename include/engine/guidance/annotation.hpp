#ifndef OSMR_GUIDANCE_ANNOTATION_HPP
#define OSMR_GUIDANCE_ANNOTATION_HPP

#include "engine/guidance/route_step.hpp"
#include "engine/guidance/annotation.hpp"

#include <vector>

namespace osrm
{
namespace engine
{
namespace guidance
{

struct Annotation
{
    std::vector<double> durations;
    std::vector<double> distances;
    std::vector<uint8_t> datasources;
    std::vector<OSMNodeID> nodes;
};
}
}
}

#endif
