/*

Copyright (c) 2014, Project OSRM, Dennis Luxen, others
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list
of conditions and the following disclaimer.
Redistributions in binary form must reproduce the above copyright notice, this
list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/
#ifndef GEOMETRY_COMPRESSOR_HPP
#define GEOMETRY_COMPRESSOR_HPP

#include "../typedefs.h"

#include "speed_profile.hpp"
#include "../data_structures/node_based_graph.hpp"

#include <memory>
#include <unordered_set>

class CompressedEdgeContainer;
class RestrictionMap;

class GraphCompressor
{
    using EdgeData = NodeBasedDynamicGraph::EdgeData;

public:
    GraphCompressor(const SpeedProfileProperties& speed_profile);

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
