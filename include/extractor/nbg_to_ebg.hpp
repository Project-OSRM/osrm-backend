#ifndef OSRM_EXTRACTOR_NBG_TO_EBG_HPP
#define OSRM_EXTRACTOR_NBG_TO_EBG_HPP

#include "util/typedefs.hpp"

namespace osrm
{
namespace extractor
{

// Mapping betweenn the node based graph u,v nodes and the edge based graph head,tail edge ids.
// Required in the osrm-partition tool to translate from a nbg partition to a ebg partition.
struct NBGToEBG
{
    NodeID u, v;
    NodeID forward_ebg_node, backward_ebg_node;
};
} // namespace extractor
} // namespace osrm

#endif
