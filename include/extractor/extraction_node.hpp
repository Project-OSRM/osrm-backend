#ifndef EXTRACTION_NODE_HPP
#define EXTRACTION_NODE_HPP

#include "traffic_lights.hpp"
#include <cstdint>

namespace osrm
{
namespace extractor
{


struct ExtractionNode
{
    ExtractionNode() : traffic_lights(TrafficLightClass::NONE), barrier(false) {}
    void clear()
    {
        traffic_lights = TrafficLightClass::NONE;
        stop_sign = StopSign::Direction::NONE;
        give_way = GiveWay::Direction::NONE;
        barrier = false;
    }
    TrafficLightClass::Direction traffic_lights;
    bool barrier;


    StopSign::Direction stop_sign;
    GiveWay::Direction give_way;
};
} // namespace extractor
} // namespace osrm

#endif // EXTRACTION_NODE_HPP
