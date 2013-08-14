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

#include "../typedefs.h"
#include "../DataStructures/DeallocatingVector.h"
#include "../DataStructures/DynamicGraph.h"
#include "../Extractor/ExtractorStructs.h"
#include "../DataStructures/HashTable.h"
#include "../DataStructures/ImportEdge.h"
#include "../DataStructures/MercatorUtil.h"
#include "../DataStructures/QueryEdge.h"
#include "../DataStructures/Percent.h"
#include "../DataStructures/TurnInstructions.h"
#include "../Util/LuaUtil.h"
#include "../Util/SimpleLogger.h"

#include <stxxl.h>

#include <boost/foreach.hpp>
#include <boost/make_shared.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>

#include <cstdlib>

#include <algorithm>
#include <queue>
#include <vector>

class EdgeBasedGraphFactory : boost::noncopyable {
public:
    struct EdgeBasedNode {
        EdgeBasedNode() :
            id(INT_MAX),
            lat1(INT_MAX),
            lat2(INT_MAX),
            lon1(INT_MAX),
            lon2(INT_MAX >> 1),
            belongsToTinyComponent(false),
            nameID(UINT_MAX),
            weight(UINT_MAX >> 1),
            ignoreInGrid(false)
        { }
        bool operator<(const EdgeBasedNode & other) const {
            return other.id < id;
        }

        bool operator==(const EdgeBasedNode & other) const {
            return id == other.id;
        }

        inline _Coordinate Centroid() const {
            _Coordinate centroid;
            //The coordinates of the midpoint are given by:
            //x = (x1 + x2) /2 and y = (y1 + y2) /2.
            centroid.lon = (std::min(lon1, lon2) + std::max(lon1, lon2))/2;
            centroid.lat = (std::min(lat1, lat2) + std::max(lat1, lat2))/2;
            return centroid;
        }

        inline bool isIgnored() const {
            return ignoreInGrid;
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
        SpeedProfileProperties()  : trafficSignalPenalty(0), uTurnPenalty(0), has_turn_penalty_function(false) {}
        int trafficSignalPenalty;
        int uTurnPenalty;
        bool has_turn_penalty_function;
    } speedProfile;

    explicit EdgeBasedGraphFactory(
        int nodes,
        std::vector<ImportEdge> & inputEdges,
        std::vector<NodeID> & _bollardNodes,
        std::vector<NodeID> & trafficLights,
        std::vector<TurnRestriction> & inputRestrictions,
        std::vector<NodeInfo> & nI,
        SpeedProfileProperties speedProfile
    );

    void Run(const char * originalEdgeDataFilename, lua_State *myLuaState);
    void GetEdgeBasedEdges( DeallocatingVector< EdgeBasedEdge >& edges );
    void GetEdgeBasedNodes( std::vector< EdgeBasedNode> & nodes);
    void GetOriginalEdgeData( std::vector< OriginalEdgeData> & originalEdgeData);
    TurnInstruction AnalyzeTurn(const NodeID u, const NodeID v, const NodeID w) const;
    int GetTurnPenalty(const NodeID u, const NodeID v, const NodeID w, lua_State *myLuaState) const;

    unsigned GetNumberOfNodes() const;

private:
    struct _NodeBasedEdgeData {
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

    typedef DynamicGraph< _NodeBasedEdgeData > _NodeBasedDynamicGraph;
    typedef _NodeBasedDynamicGraph::InputEdge _NodeBasedEdge;
    std::vector<NodeInfo>               inputNodeInfoList;
    unsigned numberOfTurnRestrictions;

    boost::shared_ptr<_NodeBasedDynamicGraph>   _nodeBasedGraph;
    boost::unordered_set<NodeID>          _barrierNodes;
    boost::unordered_set<NodeID>          _trafficLights;

    typedef std::pair<NodeID, NodeID> RestrictionSource;
    typedef std::pair<NodeID, bool>   RestrictionTarget;
    typedef std::vector<RestrictionTarget> EmanatingRestrictionsVector;
    typedef boost::unordered_map<RestrictionSource, unsigned > RestrictionMap;
    std::vector<EmanatingRestrictionsVector> _restrictionBucketVector;
    RestrictionMap _restrictionMap;

    DeallocatingVector<EdgeBasedEdge>   edgeBasedEdges;
    std::vector<EdgeBasedNode>   edgeBasedNodes;

    NodeID CheckForEmanatingIsOnlyTurn(const NodeID u, const NodeID v) const;
    bool CheckIfTurnIsRestricted(const NodeID u, const NodeID v, const NodeID w) const;
    void InsertEdgeBasedNode(
            _NodeBasedDynamicGraph::EdgeIterator e1,
            _NodeBasedDynamicGraph::NodeIterator u,
            _NodeBasedDynamicGraph::NodeIterator v,
            bool belongsToTinyComponent);
    template<class CoordinateT>
    double GetAngleBetweenTwoEdges(const CoordinateT& A, const CoordinateT& C, const CoordinateT& B) const;

};

#endif /* EDGEBASEDGRAPHFACTORY_H_ */
