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

#ifndef JSON_DESCRIPTOR_H_
#define JSON_DESCRIPTOR_H_

#include <boost/foreach.hpp>

#include "BaseDescriptor.h"
#include "DescriptionFactory.h"
#include "../DataStructures/SegmentInformation.h"
#include "../DataStructures/TurnInstructions.h"
#include "../Util/StringUtil.h"

template<class SearchEngineT>
class JSONDescriptor : public BaseDescriptor<SearchEngineT>{
private:
    _DescriptorConfig config;
    _RouteSummary summary;
    DescriptionFactory descriptionFactory;
    std::string tmp;
    _Coordinate current;

public:
    JSONDescriptor() {}
    void SetConfig(const _DescriptorConfig & c) { config = c; }

    void Run(http::Reply & reply, RawRouteData &rawRoute, PhantomNodes &phantomNodes, SearchEngineT &sEngine, unsigned durationOfTrip) {
        WriteHeaderToOutput(reply.content);
        //We do not need to do much, if there is no route ;-)

        if(durationOfTrip != INT_MAX && rawRoute.routeSegments.size() > 0) {
            summary.startName = sEngine.GetEscapedNameForNameID(phantomNodes.startPhantom.nodeBasedEdgeNameID);
            descriptionFactory.SetStartSegment(phantomNodes.startPhantom);
            summary.destName = sEngine.GetEscapedNameForNameID(phantomNodes.targetPhantom.nodeBasedEdgeNameID);
            reply.content += "0,"
                    "\"status_message\": \"Found route between points\",";
            for(unsigned segmentIdx = 0; segmentIdx < rawRoute.routeSegments.size(); segmentIdx++) {
                const std::vector< _PathData > & path = rawRoute.routeSegments[segmentIdx];
                BOOST_FOREACH(_PathData pathData, path) {
                    sEngine.GetCoordinatesForNodeID(pathData.node, current);
                    descriptionFactory.AppendSegment(current, pathData );
                }
                //TODO: Add via points
            }
            descriptionFactory.SetEndSegment(phantomNodes.targetPhantom);
        } else {
            //no route found
            reply.content += "207,"
                    "\"status_message\": \"Cannot find route between points\",";
        }

        summary.BuildDurationAndLengthStrings(descriptionFactory.Run(), durationOfTrip);

        reply.content += "\"route_summary\": {"
                "\"total_distance\":";
        reply.content += summary.lengthString;
        reply.content += ","
                "\"total_time\":";
        reply.content += summary.durationString;
        reply.content += ","
                "\"start_point\":\"";
        reply.content += summary.startName;
        reply.content += "\","
                "\"end_point\":\"";
        reply.content += summary.destName;
        reply.content += "\"";
        reply.content += "},";
        reply.content += "\"route_geometry\": ";
        if(config.geometry) {
            if(config.encodeGeometry)
                descriptionFactory.AppendEncodedPolylineString(reply.content, config.encodeGeometry);
        } else {
            reply.content += "[]";
        }

        reply.content += ","
                "\"route_instructions\": [";
        if(config.instructions) {
            unsigned prefixSumOfNecessarySegments = 0;
            std::string tmpDist, tmpLength, tmp;
            //Fetch data from Factory and generate a string from it.
            BOOST_FOREACH(SegmentInformation segment, descriptionFactory.pathDescription) {
                //["instruction","streetname",length,position,time,"length","earth_direction",azimuth]
                if(0 != segment.turnInstruction) {
                    if(0 != prefixSumOfNecessarySegments)
                        reply.content += ",";
                    reply.content += "[\"";
                    reply.content += TurnInstructions.TurnStrings[segment.turnInstruction];
                    reply.content += "\",\"";
                    reply.content += sEngine.GetEscapedNameForNameID(segment.nameID);
                    reply.content += "\",";
                    intToString(segment.length, tmpDist);
                    reply.content += tmpDist;
                    reply.content += ",";
                    intToString(prefixSumOfNecessarySegments, tmpLength);
                    reply.content += tmpLength;
                    reply.content += ",";
                    intToString(segment.duration, tmp);
                    reply.content += ",\"";
                    reply.content += tmpLength;
                    //TODO: fix heading
                    reply.content += "\",\"NE\",22.5";
                    reply.content += "]";
                }
                if(segment.necessary)
                    ++prefixSumOfNecessarySegments;
            }
            //            descriptionFactory.AppendRouteInstructionString(reply.content);

        }
        reply.content += "],";
        //list all viapoints so that the client may display it
        reply.content += "\"via_points\":[";
        for(unsigned segmentIdx = 1; (true == config.geometry) && (segmentIdx < rawRoute.segmentEndCoordinates.size()); segmentIdx++) {
            if(segmentIdx > 1)
                reply.content += ",";
            reply.content += "[";
            if(rawRoute.segmentEndCoordinates[segmentIdx].startPhantom.location.isSet())
                convertInternalReversedCoordinateToString(rawRoute.segmentEndCoordinates[segmentIdx].startPhantom.location, tmp);
            else
                convertInternalReversedCoordinateToString(rawRoute.rawViaNodeCoordinates[segmentIdx], tmp);
            reply.content += tmp;
            reply.content += "]";
        }
        reply.content += "],"
                "\"transactionId\": \"OSRM Routing Engine JSON Descriptor (v0.2)\"";
        reply.content += "}";
    }

    void WriteHeaderToOutput(std::string & output) {
        output += "{"
                "\"version\": 0.3,"
                "\"status\":";
    }
};
#endif /* JSON_DESCRIPTOR_H_ */
