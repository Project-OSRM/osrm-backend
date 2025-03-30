#ifndef EXTRACTION_NODE_HPP
#define EXTRACTION_NODE_HPP

#include <osmium/osm/node.hpp>

namespace osrm::extractor
{

struct ExtractionNode
{
    ExtractionNode() {}

    // the current node
    const osmium::Node *node;
};
} // namespace osrm::extractor

#endif // EXTRACTION_NODE_HPP
