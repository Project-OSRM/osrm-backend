#ifndef EXTRACTION_NODE_HPP
#define EXTRACTION_NODE_HPP

namespace osrm
{
namespace extractor
{

struct ExtractionNode
{
    ExtractionNode() : traffic_lights(false), barrier(false), access_restricted(false) {}
    void clear() { traffic_lights = barrier = access_restricted = false; }
    bool traffic_lights;
    bool barrier;
    bool access_restricted;
};
}
}

#endif // EXTRACTION_NODE_HPP
