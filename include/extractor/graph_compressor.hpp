#ifndef GEOMETRY_COMPRESSOR_HPP
#define GEOMETRY_COMPRESSOR_HPP

#include "util/typedefs.hpp"

#include "util/node_based_graph.hpp"

#include <memory>
#include <unordered_set>

namespace osrm
{
namespace extractor
{

class CompressedEdgeContainer;
class RestrictionMap;

class GraphCompressor
{
    using EdgeData = util::NodeBasedDynamicGraph::EdgeData;

  public:
    void Compress(const std::unordered_set<NodeID> &barrier_nodes,
                  const std::unordered_set<NodeID> &traffic_lights,
                  RestrictionMap &restriction_map,
                  util::NodeBasedDynamicGraph &graph,
                  CompressedEdgeContainer &geometry_compressor);

  private:
    void PrintStatistics(unsigned original_number_of_nodes,
                         unsigned original_number_of_edges,
                         const util::NodeBasedDynamicGraph &graph) const;
};
}
}

#endif
