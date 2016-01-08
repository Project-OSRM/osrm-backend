#ifndef EXTRACTION_NODE_HPP
#define EXTRACTION_NODE_HPP

namespace osrm
{
namespace extractor
{

struct ExtractionNode
{
    ExtractionNode() : traffic_lights(false), barrier(false) {}
    void clear() { traffic_lights = barrier = false; }
    bool traffic_lights;
    bool barrier;
};
}
}

#endif // EXTRACTION_NODE_HPP
