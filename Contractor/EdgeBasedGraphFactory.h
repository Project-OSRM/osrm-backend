/*
 open source routing machine
 Copyright (C) Dennis Luxen, others 2010

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU AFFERO General Public License as published by
 the Free Software Foundation; either version 3 of the License, or
 any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU Affero General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 or see http://www.gnu.org/licenses/agpl.txt.
 */

/*
 * This class constructs the edge base representation of a graph from a given node based edge list
 */

#ifndef EDGEBASEDGRAPHFACTORY_H_
#define EDGEBASEDGRAPHFACTORY_H_

#include <algorithm>
#include <queue>
#include <vector>
#include <stxxl.h>
#include <cstdlib>

#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>


#include "../typedefs.h"
#include "../DataStructures/DeallocatingVector.h"
#include "../DataStructures/DynamicGraph.h"
#include "../Extractor/ExtractorStructs.h"
#include "../DataStructures/HashTable.h"
#include "../DataStructures/ImportEdge.h"
#include "../DataStructures/QueryEdge.h"
#include "../DataStructures/Percent.h"
#include "../DataStructures/TurnInstructions.h"
#include "../Util/BaseConfiguration.h"

class EdgeBasedGraphFactory {
private:
    struct _NodeBasedEdgeData {
        int distance;
        unsigned edgeBasedNodeID;
        unsigned nameID:31;
        bool shortcut:1;
        bool forward:1;
        bool backward:1;
        bool roundabout:1;
        bool ignoreInGrid:1;
        short type;
        bool isAccessRestricted;
    };

    struct _EdgeBasedEdgeData {
        int distance;
        unsigned via;
        unsigned nameID;
        bool forward;
        bool backward;
        TurnInstruction turnInstruction;
    };

    typedef DynamicGraph< _NodeBasedEdgeData > _NodeBasedDynamicGraph;
    typedef _NodeBasedDynamicGraph::InputEdge _NodeBasedEdge;
    std::vector<NodeInfo>               inputNodeInfoList;
    unsigned numberOfTurnRestrictions;
public:
    struct EdgeBasedNode {
        bool operator<(const EdgeBasedNode & other) const {
            return other.id < id;
        }
        bool operator==(const EdgeBasedNode & other) const {
            return id == other.id;
        }
        NodeID id;
        int lat1;
        int lat2;
        int lon1;
        int lon2:31;
        bool belongsToTinyComponent:1;
        NodeID nameID;
        unsigned weight:31;
        bool ignoreInGrid:1;
    };


    struct SpeedProfileProperties{
        SpeedProfileProperties()  : trafficSignalPenalty(0), uTurnPenalty(0) {}
        int trafficSignalPenalty;
        int uTurnPenalty;
    } speedProfile;
private:
    boost::shared_ptr<_NodeBasedDynamicGraph>   _nodeBasedGraph;
    boost::unordered_map<NodeID, bool>          _barrierNodes;
    boost::unordered_map<NodeID, bool>          _trafficLights;

    typedef std::pair<NodeID, NodeID> RestrictionSource;
    typedef std::pair<NodeID, bool>   RestrictionTarget;
    typedef std::vector<RestrictionTarget> EmanatingRestrictionsVector;
    typedef boost::unordered_map<RestrictionSource, unsigned > RestrictionMap;
    std::vector<EmanatingRestrictionsVector> _restrictionBucketVector;
    RestrictionMap _restrictionMap;


    DeallocatingVector<EdgeBasedEdge>   edgeBasedEdges;
    DeallocatingVector<EdgeBasedNode>   edgeBasedNodes;
    std::vector<OriginalEdgeData>       originalEdgeData;

    NodeID CheckForEmanatingIsOnlyTurn(const NodeID u, const NodeID v) const;
    bool CheckIfTurnIsRestricted(const NodeID u, const NodeID v, const NodeID w) const;
    void InsertEdgeBasedNode(
            _NodeBasedDynamicGraph::EdgeIterator e1,
            _NodeBasedDynamicGraph::NodeIterator u,
            _NodeBasedDynamicGraph::NodeIterator v,
            bool belongsToTinyComponent);
    template<class CoordinateT>
    double GetAngleBetweenTwoEdges(const CoordinateT& A, const CoordinateT& C, const CoordinateT& B) const;
//    SRTMLookup srtmLookup;


public:
    template< class InputEdgeT >
    explicit EdgeBasedGraphFactory(int nodes, std::vector<InputEdgeT> & inputEdges, std::vector<NodeID> & _bollardNodes, std::vector<NodeID> & trafficLights, std::vector<_Restriction> & inputRestrictions, std::vector<NodeInfo> & nI, SpeedProfileProperties speedProfile);

    void Run(const char * originalEdgeDataFilename);
    void GetEdgeBasedEdges( DeallocatingVector< EdgeBasedEdge >& edges );
    void GetEdgeBasedNodes( DeallocatingVector< EdgeBasedNode> & nodes);
    void GetOriginalEdgeData( std::vector< OriginalEdgeData> & originalEdgeData);
    TurnInstruction AnalyzeTurn(const NodeID u, const NodeID v, const NodeID w) const;
    unsigned GetNumberOfNodes() const;
};

#endif /* EDGEBASEDGRAPHFACTORY_H_ */
