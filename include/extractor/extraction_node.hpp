#ifndef EXTRACTION_NODE_HPP
#define EXTRACTION_NODE_HPP

namespace osrm
{
namespace extractor
{

struct ExtractionNode
{
    ExtractionNode() { clear(); }

    void clear() { traffic_lights = barrier = exit = false; }

    bool traffic_lights;
    bool barrier;
    bool exit;
};
}
}

#endif // EXTRACTION_NODE_HPP
