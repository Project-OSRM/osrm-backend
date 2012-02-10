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
#include "../Util/Azimuth.h"
#include "../Util/StringUtil.h"

template<class SearchEngineT>
class JSONDescriptor : public BaseDescriptor<SearchEngineT>{
private:
	_DescriptorConfig config;
	DescriptionFactory descriptionFactory;
	_Coordinate current;

	struct {
		int startIndex;
		int nameID;
		int leaveAtExit;
	} roundAbout;

public:
	JSONDescriptor() {}
	void SetConfig(const _DescriptorConfig & c) { config = c; }

	void Run(http::Reply & reply, RawRouteData &rawRoute, PhantomNodes &phantomNodes, SearchEngineT &sEngine, const unsigned durationOfTrip) {
		WriteHeaderToOutput(reply.content);
		if(durationOfTrip != INT_MAX) {
			descriptionFactory.SetStartSegment(phantomNodes.startPhantom);
			reply.content += "0,"
			        "\"status_message\": \"Found route between points\",";

			//Get all the coordinates for the computed route
			BOOST_FOREACH(_PathData & pathData, rawRoute.computedRouted) {
			    sEngine.GetCoordinatesForNodeID(pathData.node, current);
			    descriptionFactory.AppendSegment(current, pathData );
			}
			descriptionFactory.SetEndSegment(phantomNodes.targetPhantom);
		} else {
			//We do not need to do much, if there is no route ;-)
			reply.content += "207,"
					"\"status_message\": \"Cannot find route between points\",";
		}

		descriptionFactory.Run(config.z, durationOfTrip);

		reply.content += "\"route_summary\": {"
				"\"total_distance\":";
		reply.content += descriptionFactory.summary.lengthString;
		reply.content += ","
				"\"total_time\":";
		reply.content += descriptionFactory.summary.durationString;
		reply.content += ","
				"\"start_point\":\"";
		reply.content += sEngine.GetEscapedNameForNameID(descriptionFactory.summary.startName);
		reply.content += "\","
				"\"end_point\":\"";
		reply.content += sEngine.GetEscapedNameForNameID(descriptionFactory.summary.destName);
		reply.content += "\"";
		reply.content += "},";
		reply.content += "\"route_geometry\": ";
		if(config.geometry) {
		    descriptionFactory.AppendEncodedPolylineString(reply.content, config.encodeGeometry);
		} else {
			reply.content += "[]";
		}

		reply.content += ","
				"\"route_instructions\": [";
		if(config.instructions) {
			//Segment information has following format:
			//["instruction","streetname",length,position,time,"length","earth_direction",azimuth]
			//Example: ["Turn left","High Street",200,4,10,"200m","NE",22.5]
			//See also: http://developers.cloudmade.com/wiki/navengine/JSON_format
			unsigned prefixSumOfNecessarySegments = 0;
			roundAbout.leaveAtExit = 0;
			roundAbout.nameID = 0;
			std::string tmpDist, tmpLength, tmpDuration, tmpBearing;
			//Fetch data from Factory and generate a string from it.
			BOOST_FOREACH(SegmentInformation & segment, descriptionFactory.pathDescription) {
				if(TurnInstructions.TurnIsNecessary( segment.turnInstruction) ) {
					if(TurnInstructions.EnterRoundAbout == segment.turnInstruction) {
						roundAbout.nameID = segment.nameID;
						roundAbout.startIndex = prefixSumOfNecessarySegments;
					} else {
						if(0 != prefixSumOfNecessarySegments)
							reply.content += ",";

						reply.content += "[\"";
						if(TurnInstructions.LeaveRoundAbout == segment.turnInstruction) {
							reply.content += TurnInstructions.TurnStrings[TurnInstructions.EnterRoundAbout];
							reply.content += " and leave at ";
							reply.content += TurnInstructions.Ordinals[roundAbout.leaveAtExit+1];
							reply.content += " exit";
							roundAbout.leaveAtExit = 0;
						} else {
							reply.content += TurnInstructions.TurnStrings[segment.turnInstruction];
						}
						reply.content += "\",\"";
						reply.content += sEngine.GetEscapedNameForNameID(segment.nameID);
						reply.content += "\",";
						intToString(segment.length, tmpDist);
						reply.content += tmpDist;
						reply.content += ",";
						intToString(prefixSumOfNecessarySegments, tmpLength);
						reply.content += tmpLength;
						reply.content += ",";
						intToString(segment.duration, tmpDuration);
						reply.content += tmpDuration;
						reply.content += ",\"";
						intToString(segment.length, tmpLength);
						reply.content += tmpLength;
						reply.content += "m\",\"";
						reply.content += Azimuth::Get(segment.bearing);
						reply.content += "\",";
						doubleToStringWithTwoDigitsBehindComma(segment.bearing, tmpBearing);
						reply.content += tmpBearing;
			            reply.content += "]";
					}
				} else if(TurnInstructions.StayOnRoundAbout == segment.turnInstruction) {
					++roundAbout.leaveAtExit;
				}
				if(segment.necessary)
					++prefixSumOfNecessarySegments;
			}
		}
		reply.content += "],";
		//list all viapoints so that the client may display it
		reply.content += "\"via_points\":[";
		if(true == config.geometry) {
			std::string tmp;
			for(unsigned segmentIdx = 1; segmentIdx < rawRoute.segmentEndCoordinates.size(); ++segmentIdx) {
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
		}
		reply.content += "],"
				"\"transactionId\": \"OSRM Routing Engine JSON Descriptor (v0.3)\"";
		reply.content += "}";
	}

	inline void WriteHeaderToOutput(std::string & output) {
		output += "{"
				"\"version\": 0.3,"
				"\"status\":";
	}
};
#endif /* JSON_DESCRIPTOR_H_ */
