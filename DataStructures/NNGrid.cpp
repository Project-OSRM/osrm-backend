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

#include "../typedefs.h"
#include "NNGrid.h"
#include "NodeInformationHelpDesk.h"

bool NNGrid::FindPhantomNodeForCoordinate( const _Coordinate & location, PhantomNode & resultNode, const unsigned zoomLevel) {
    bool ignoreTinyComponents = (zoomLevel <= 14);
//        INFO("Coordinate: " << location << ", zoomLevel: " << zoomLevel << ", ignoring tinyComponentents: " << (ignoreTinyComponents ? "yes" : "no"));
//        double time1 = get_timestamp();
    bool foundNode = false;
    const _Coordinate startCoord(100000*(lat2y(static_cast<double>(location.lat)/100000.)), location.lon);
    /** search for point on edge close to source */
    const unsigned fileIndex = GetFileIndexForLatLon(startCoord.lat, startCoord.lon);
    std::vector<_GridEdge> candidates;
    const int lowerBoundForLoop = (fileIndex < 32768 ? 0 : -32768);
    for(int j = lowerBoundForLoop; (j < (32768+1)) && (fileIndex != UINT_MAX); j+=32768) {
        for(int i = -1; i < 2; ++i){
//                unsigned oldSize = candidates.size();
            GetContentsOfFileBucketEnumerated(fileIndex+i+j, candidates);
//                INFO("Getting fileIndex=" << fileIndex+i+j << " with " << candidates.size() - oldSize << " candidates");
        }
    }
//        INFO("looked up " << candidates.size());
    _GridEdge smallestEdge;
    _Coordinate tmp, edgeStartCoord, edgeEndCoord;
    double dist = std::numeric_limits<double>::max();
    double r, tmpDist;

    BOOST_FOREACH(const _GridEdge & candidate, candidates) {
        if(candidate.belongsToTinyComponent && ignoreTinyComponents)
            continue;
        r = 0.;
        tmpDist = ComputeDistance(startCoord, candidate.startCoord, candidate.targetCoord, tmp, &r);
//            INFO("dist " << startCoord << "->[" << candidate.startCoord << "-" << candidate.targetCoord << "]=" << tmpDist );
//            INFO("Looking at edge " << candidate.edgeBasedNode << " at distance " << tmpDist);
        if(tmpDist < dist && !DoubleEpsilonCompare(dist, tmpDist)) {
//                INFO("a) " << candidate.edgeBasedNode << ", dist: " << tmpDist << ", tinyCC: " << (candidate.belongsToTinyComponent ? "yes" : "no"));
            dist = tmpDist;
            resultNode.edgeBasedNode = candidate.edgeBasedNode;
            resultNode.nodeBasedEdgeNameID = candidate.nameID;
            resultNode.mode1 = candidate.mode;
            resultNode.mode2 = 0;
            resultNode.weight1 = candidate.weight;
            resultNode.weight2 = INT_MAX;
            resultNode.location.lat = tmp.lat;
            resultNode.location.lon = tmp.lon;
            edgeStartCoord = candidate.startCoord;
            edgeEndCoord = candidate.targetCoord;
            foundNode = true;
            smallestEdge = candidate;
            //}  else if(tmpDist < dist) {
            //INFO("a) ignored " << candidate.edgeBasedNode << " at distance " << std::fabs(dist - tmpDist));
        } else if(DoubleEpsilonCompare(dist, tmpDist) && 1 == std::abs(static_cast<int>(candidate.edgeBasedNode)-static_cast<int>(resultNode.edgeBasedNode) )  && CoordinatesAreEquivalent(edgeStartCoord, candidate.startCoord, edgeEndCoord, candidate.targetCoord)) {
            resultNode.edgeBasedNode = std::min(candidate.edgeBasedNode, resultNode.edgeBasedNode);
            resultNode.weight2 = candidate.weight;
            resultNode.mode2 = candidate.mode;
            //INFO("b) " << candidate.edgeBasedNode << ", dist: " << tmpDist);
        }
    }

    //        INFO("startcoord: " << smallestEdge.startCoord << ", tgtcoord" <<  smallestEdge.targetCoord << "result: " << newEndpoint);
    //        INFO("length of old edge: " << ApproximateDistance(smallestEdge.startCoord, smallestEdge.targetCoord));
    //        INFO("Length of new edge: " << ApproximateDistance(smallestEdge.startCoord, newEndpoint));
    //        assert(!resultNode.isBidirected() || (resultNode.weight1 == resultNode.weight2));
    //        if(resultNode.weight1 != resultNode.weight2) {
    //            INFO("-> Weight1: " << resultNode.weight1 << ", weight2: " << resultNode.weight2);
    //            INFO("-> node: " << resultNode.edgeBasedNode << ", bidir: " << (resultNode.isBidirected() ? "yes" : "no"));
    //        }

//        INFO("startCoord: " << smallestEdge.startCoord << "; targetCoord: " << smallestEdge.targetCoord << "; newEndpoint: " << resultNode.location);
    const double ratio = (foundNode ? std::min(1., ApproximateDistance(smallestEdge.startCoord, resultNode.location)/ApproximateDistance(smallestEdge.startCoord, smallestEdge.targetCoord)) : 0);
    resultNode.location.lat = round(100000.*(y2lat(static_cast<double>(resultNode.location.lat)/100000.)));
//        INFO("Length of vector: " << ApproximateDistance(smallestEdge.startCoord, resultNode.location)/ApproximateDistance(smallestEdge.startCoord, smallestEdge.targetCoord));
    //Hack to fix rounding errors and wandering via nodes.
    if(std::abs(location.lon - resultNode.location.lon) == 1)
        resultNode.location.lon = location.lon;
    if(std::abs(location.lat - resultNode.location.lat) == 1)
        resultNode.location.lat = location.lat;

    resultNode.weight1 *= ratio;
    if(INT_MAX != resultNode.weight2) {
        resultNode.weight2 *= (1.-ratio);
    }
    resultNode.ratio = ratio;
//        INFO("start: " << edgeStartCoord << ", end: " << edgeEndCoord);
//        INFO("selected node: " << resultNode.edgeBasedNode << ", bidirected: " << (resultNode.isBidirected() ? "yes" : "no"));
//        INFO("New weight1: " << resultNode.weight1 << ", new weight2: " << resultNode.weight2 << ", ratio: " << ratio);
//       INFO("distance to input coordinate: " << ApproximateDistance(location, resultNode.location) <<  "\n--");
//        double time2 = get_timestamp();
//        INFO("NN-Lookup in " << 1000*(time2-time1) << "ms");
    
    return foundNode;
}

