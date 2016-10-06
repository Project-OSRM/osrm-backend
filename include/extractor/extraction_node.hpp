#ifndef EXTRACTION_NODE_HPP
#define EXTRACTION_NODE_HPP

#include "extractor/road_signs.hpp"

#include <cstdint>

namespace osrm
{
namespace extractor
{

struct ExtractionNode
{
    ExtractionNode()
        : traffic_lights(false), barrier(false), stop_sign(StopSign::No),
          give_way_sign(GiveWaySign::No)
    {
    }

    void clear()
    {
        traffic_lights = false;
        barrier = false;
        stop_sign = StopSign::No;
        give_way_sign = GiveWaySign::No;
    }

    bool traffic_lights;
    bool barrier;

    StopSign::State stop_sign;
    GiveWaySign::State give_way_sign;
};
}
}

#endif // EXTRACTION_NODE_HPP
