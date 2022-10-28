#ifndef EXTRACTION_NODE_HPP
#define EXTRACTION_NODE_HPP

#include "traffic_flow_control_nodes.hpp"
#include <cstdint>

namespace osrm
{
namespace extractor
{

struct ExtractionNode
{
    ExtractionNode() : traffic_lights(TrafficFlowControlNodeDirection::NONE), barrier(false) {}
    void clear()
    {
        traffic_lights = TrafficFlowControlNodeDirection::NONE;
        stop_sign = TrafficFlowControlNodeDirection::NONE;
        give_way = TrafficFlowControlNodeDirection::NONE;
        barrier = false;
    }
    TrafficFlowControlNodeDirection traffic_lights;
    bool barrier;

    TrafficFlowControlNodeDirection stop_sign;
    TrafficFlowControlNodeDirection give_way;
};
} // namespace extractor
} // namespace osrm

#endif // EXTRACTION_NODE_HPP
