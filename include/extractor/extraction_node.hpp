#ifndef EXTRACTION_NODE_HPP
#define EXTRACTION_NODE_HPP

namespace osrm
{
namespace extractor
{

struct ExtractionNode
{
    ExtractionNode()
        : traffic_lights(false), barrier(false), is_all_way_stop(false), is_minor_stop(false)
    {
    }
    void clear() { traffic_lights = barrier = is_all_way_stop = is_minor_stop = false; }
    bool traffic_lights;
    bool barrier;

    bool is_all_way_stop;
    bool is_minor_stop;
};
}
}

#endif // EXTRACTION_NODE_HPP
