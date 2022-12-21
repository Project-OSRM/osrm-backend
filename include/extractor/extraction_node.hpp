#ifndef EXTRACTION_NODE_HPP
#define EXTRACTION_NODE_HPP

#include "traffic_lights.hpp"

namespace osrm::extractor
{

struct ExtractionNode
{
    ExtractionNode() : traffic_lights(TrafficLightClass::NONE), barrier(false) {}
    void clear()
    {
        traffic_lights = TrafficLightClass::NONE;
        barrier = false;
    }
    TrafficLightClass::Direction traffic_lights;
    bool barrier;
};
} // namespace osrm::extractor

#endif // EXTRACTION_NODE_HPP
