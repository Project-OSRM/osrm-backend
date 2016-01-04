#ifndef GEOMETRY_COMPRESSOR_HPP
#define GEOMETRY_COMPRESSOR_HPP

#include "util/typedefs.hpp"

#include "extractor/speed_profile.hpp"
#include "util/node_based_graph.hpp"

#include <memory>
#include <unordered_set>

class CompressedEdgeContainer;
class RestrictionMap;

class GraphCompressor
{
    using EdgeData = NodeBasedDynamicGraph::EdgeData;

public:
  GraphCompressor(SpeedProfileProperties speed_profile);

    void Compress(const std::unordered_set<NodeID>& barrier_nodes,
                  const std::unordered_set<NodeID>& traffic_lights,
                  RestrictionMap& restriction_map,
                  NodeBasedDynamicGraph& graph,
                  CompressedEdgeContainer& geometry_compressor);
private:

   void PrintStatistics(unsigned original_number_of_nodes,
                        unsigned original_number_of_edges,
                        const NodeBasedDynamicGraph& graph) const;

    SpeedProfileProperties speed_profile;
};

#endif
