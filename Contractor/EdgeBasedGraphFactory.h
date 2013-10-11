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

//  This class constructs the edge-expanded routing graph

#ifndef EDGEBASEDGRAPHFACTORY_H_
#define EDGEBASEDGRAPHFACTORY_H_

#include "../typedefs.h"
#include "../DataStructures/DeallocatingVector.h"
#include "../DataStructures/DynamicGraph.h"
#include "../DataStructures/EdgeBasedNode.h"
#include "../Extractor/ExtractorStructs.h"
#include "../DataStructures/HashTable.h"
#include "../DataStructures/ImportEdge.h"
#include "../DataStructures/QueryEdge.h"
#include "../DataStructures/Percent.h"
#include "../DataStructures/TurnInstructions.h"
#include "../Util/LuaUtil.h"
#include "../Util/SimpleLogger.h"

#include <boost/foreach.hpp>
#include <boost/make_shared.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>

#include <algorithm>
#include <fstream>
#include <queue>
#include <vector>

class EdgeBasedGraphFactory : boost::noncopyable {
public:
    struct SpeedProfileProperties{
        SpeedProfileProperties() :
            trafficSignalPenalty(0),
            uTurnPenalty(0),
            has_turn_penalty_function(false)
        { }

        int trafficSignalPenalty;
        int uTurnPenalty;
        bool has_turn_penalty_function;
    } speed_profile;

    explicit EdgeBasedGraphFactory(
        int number_of_nodes,
        std::vector<ImportEdge> & input_edge_list,
        std::vector<NodeID> & barrier_node_list,
        std::vector<NodeID> & traffic_light_node_list,
        std::vector<TurnRestriction> & input_restrictions_list,
        std::vector<NodeInfo> & m_node_info_list,
        SpeedProfileProperties speed_profile
    );

    void Run(const char * originalEdgeDataFilename, lua_State *myLuaState);
    void GetEdgeBasedEdges( DeallocatingVector< EdgeBasedEdge >& edges );
    void GetEdgeBasedNodes( std::vector< EdgeBasedNode> & nodes);
    void GetOriginalEdgeData( std::vector<OriginalEdgeData> & originalEdgeData);
    TurnInstruction AnalyzeTurn(
        const NodeID u,
        const NodeID v,
        const NodeID w
    ) const;
    int GetTurnPenalty(
        const NodeID u,
        const NodeID v,
        const NodeID w,
        lua_State *myLuaState
    ) const;

    unsigned GetNumberOfNodes() const;

private:
    struct NodeBasedEdgeData {
        int distance;
        unsigned edgeBasedNodeID;
        unsigned nameID;
        short type;
        bool isAccessRestricted:1;
        bool shortcut:1;
        bool forward:1;
        bool backward:1;
        bool roundabout:1;
        bool ignoreInGrid:1;
        bool contraFlow:1;
    };

    struct _EdgeBasedEdgeData {
        int distance;
        unsigned via;
        unsigned nameID;
        bool forward;
        bool backward;
        TurnInstruction turnInstruction;
    };

    unsigned m_turn_restrictions_count;

    typedef DynamicGraph<NodeBasedEdgeData>     NodeBasedDynamicGraph;
    typedef NodeBasedDynamicGraph::InputEdge    NodeBasedEdge;
    typedef NodeBasedDynamicGraph::NodeIterator NodeIterator;
    typedef NodeBasedDynamicGraph::EdgeIterator EdgeIterator;
    typedef NodeBasedDynamicGraph::EdgeData     EdgeData;
    typedef std::pair<NodeID, NodeID>           RestrictionSource;
    typedef std::pair<NodeID, bool>             RestrictionTarget;
    typedef std::vector<RestrictionTarget>      EmanatingRestrictionsVector;
    typedef boost::unordered_map<RestrictionSource, unsigned > RestrictionMap;

    std::vector<NodeInfo>                       m_node_info_list;
    std::vector<EmanatingRestrictionsVector>    m_restriction_bucket_list;
    std::vector<EdgeBasedNode>                  m_edge_based_node_list;
    DeallocatingVector<EdgeBasedEdge>           m_edge_based_edge_list;

    boost::shared_ptr<NodeBasedDynamicGraph>    m_node_based_graph;
    boost::unordered_set<NodeID>                m_barrier_nodes;
    boost::unordered_set<NodeID>                m_traffic_lights;

    RestrictionMap                              m_restriction_map;


    NodeID CheckForEmanatingIsOnlyTurn(
        const NodeID u,
        const NodeID v
    ) const;

    bool CheckIfTurnIsRestricted(
        const NodeID u,
        const NodeID v,
        const NodeID w
    ) const;

    void InsertEdgeBasedNode(
            NodeBasedDynamicGraph::EdgeIterator e1,
            NodeBasedDynamicGraph::NodeIterator u,
            NodeBasedDynamicGraph::NodeIterator v,
            bool belongsToTinyComponent);
};

#endif /* EDGEBASEDGRAPHFACTORY_H_ */
