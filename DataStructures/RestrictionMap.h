/*

Copyright (c) 2013, Project OSRM, Dennis Luxen, others
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

#ifndef __RESTRICTION_MAP_H__
#define __RESTRICTION_MAP_H__

#include <memory>

#include "DynamicGraph.h"
#include "Restriction.h"
#include "NodeBasedGraph.h"
#include "../Util/StdHashExtensions.h"
#include "../typedefs.h"

#include <unordered_map>
#include <unordered_set>

// Efficent look up if an edge is the start + via node of a TurnRestriction
// EdgeBasedEdgeFactory decides by it if edges are inserted or geometry is compressed
class RestrictionMap
{
  public:
    RestrictionMap(const std::shared_ptr<NodeBasedDynamicGraph> &graph,
                   const std::vector<TurnRestriction> &input_restrictions_list);

    void FixupArrivingTurnRestriction(const NodeID u, const NodeID v, const NodeID w);
    void FixupStartingTurnRestriction(const NodeID u, const NodeID v, const NodeID w);
    NodeID CheckForEmanatingIsOnlyTurn(const NodeID u, const NodeID v) const;
    bool CheckIfTurnIsRestricted(const NodeID u, const NodeID v, const NodeID w) const;
    bool IsNodeAViaNode(const NodeID node) const;
    unsigned size() { return m_count; }

  private:
    bool RestrictionStartsAtNode(const NodeID node) const;
    typedef std::pair<NodeID, NodeID> RestrictionSource;
    typedef std::pair<NodeID, bool> RestrictionTarget;
    typedef std::vector<RestrictionTarget> EmanatingRestrictionsVector;
    typedef NodeBasedDynamicGraph::EdgeData EdgeData;

    unsigned m_count;
    std::shared_ptr<NodeBasedDynamicGraph> m_graph;
    //! index -> list of (target, isOnly)
    std::vector<EmanatingRestrictionsVector> m_restriction_bucket_list;
    //! maps (start, via) -> bucket index
    std::unordered_map<RestrictionSource, unsigned> m_restriction_map;
    std::unordered_set<NodeID> m_restriction_start_nodes;
    std::unordered_set<NodeID> m_no_turn_via_node_set;
};

#endif
