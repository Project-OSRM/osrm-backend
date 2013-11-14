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

#ifndef JSON_DESCRIPTOR_H_
#define JSON_DESCRIPTOR_H_

#include "BaseDescriptor.h"
#include "DescriptionFactory.h"
#include "../Algorithms/ObjectToBase64.h"
#include "../DataStructures/SegmentInformation.h"
#include "../DataStructures/TurnInstructions.h"
#include "../Util/Azimuth.h"
#include "../Util/StringUtil.h"

#include <boost/bind.hpp>
#include <boost/lambda/lambda.hpp>

#include <algorithm>

template<class DataFacadeT>
class JSONDescriptor : public BaseDescriptor<DataFacadeT> {
private:
    DescriptorConfig config;
    DescriptionFactory description_factory;
    DescriptionFactory alternateDescriptionFactory;
    FixedPointCoordinate current;
    unsigned entered_restricted_area_count;
    struct RoundAbout{
        RoundAbout() :
            start_index(INT_MAX),
            name_id(INT_MAX),
            leave_at_exit(INT_MAX)
        {}
        int start_index;
        int name_id;
        int leave_at_exit;
    } roundAbout;

    struct Segment {
        Segment() : name_id(-1), length(-1), position(-1) {}
        Segment(int n, int l, int p) : name_id(n), length(l), position(p) {}
        int name_id;
        int length;
        int position;
    };
    std::vector<Segment> shortest_path_segments, alternative_path_segments;

    struct RouteNames {
        std::string shortestPathName1;
        std::string shortestPathName2;
        std::string alternativePathName1;
        std::string alternativePathName2;
    };

public:
    JSONDescriptor() : entered_restricted_area_count(0) {}
    void SetConfig(const DescriptorConfig & c) { config = c; }

    //TODO: reorder parameters
    void Run(
        http::Reply & reply,
        const RawRouteData & raw_route_information,
        PhantomNodes & phantom_nodes,
        const DataFacadeT * facade
    ) {

        WriteHeaderToOutput(reply.content);

        if(raw_route_information.lengthOfShortestPath != INT_MAX) {
            description_factory.SetStartSegment(phantom_nodes.startPhantom);
            reply.content.push_back("0,"
                    "\"status_message\": \"Found route between points\",");

            //Get all the coordinates for the computed route
            BOOST_FOREACH(const _PathData & path_data, raw_route_information.computedShortestPath) {
                current = facade->GetCoordinateOfNode(path_data.node);
                description_factory.AppendSegment(current, path_data );
            }
            description_factory.SetEndSegment(phantom_nodes.targetPhantom);
        } else {
            //We do not need to do much, if there is no route ;-)
            reply.content.push_back("207,"
                    "\"status_message\": \"Cannot find route between points\",");
        }

        description_factory.Run(facade, config.zoom_level);
        reply.content.push_back("\"route_geometry\": ");
        if(config.geometry) {
            description_factory.AppendEncodedPolylineString(
               config.encode_geometry,
               reply.content
            );
        } else {
            reply.content.push_back("[]");
        }

        reply.content.push_back(","
                "\"route_instructions\": [");
        entered_restricted_area_count = 0;
        if(config.instructions) {
            BuildTextualDescription(
                description_factory,
                reply,
                raw_route_information.lengthOfShortestPath,
                facade,
                shortest_path_segments
            );
        } else {
            BOOST_FOREACH(
                const SegmentInformation & segment,
                description_factory.pathDescription
            ) {
                TurnInstruction current_instruction = segment.turnInstruction & TurnInstructions.InverseAccessRestrictionFlag;
                entered_restricted_area_count += (current_instruction != segment.turnInstruction);
            }
        }
        reply.content.push_back("],");
        description_factory.BuildRouteSummary(
            description_factory.entireLength,
            raw_route_information.lengthOfShortestPath - ( entered_restricted_area_count*TurnInstructions.AccessRestrictionPenalty)
        );

        reply.content.push_back("\"route_summary\":");
        reply.content.push_back("{");
        reply.content.push_back("\"total_distance\":");
        reply.content.push_back(description_factory.summary.lengthString);
        reply.content.push_back(","
                "\"total_time\":");
        reply.content.push_back(description_factory.summary.durationString);
        reply.content.push_back(","
                "\"start_point\":\"");
        reply.content.push_back(
            facade->GetEscapedNameForNameID(description_factory.summary.startName)
        );
        reply.content.push_back("\","
                "\"end_point\":\"");
        reply.content.push_back(
            facade->GetEscapedNameForNameID(description_factory.summary.destName)
        );
        reply.content.push_back("\"");
        reply.content.push_back("}");
        reply.content.push_back(",");

        //only one alternative route is computed at this time, so this is hardcoded

        if(raw_route_information.lengthOfAlternativePath != INT_MAX) {
            alternateDescriptionFactory.SetStartSegment(phantom_nodes.startPhantom);
            //Get all the coordinates for the computed route
            BOOST_FOREACH(const _PathData & path_data, raw_route_information.computedAlternativePath) {
                current = facade->GetCoordinateOfNode(path_data.node);
                alternateDescriptionFactory.AppendSegment(current, path_data );
            }
            alternateDescriptionFactory.SetEndSegment(phantom_nodes.targetPhantom);
        }
        alternateDescriptionFactory.Run(facade, config.zoom_level);

        //give an array of alternative routes
        reply.content.push_back("\"alternative_geometries\": [");
        if(config.geometry && INT_MAX != raw_route_information.lengthOfAlternativePath) {
            //Generate the linestrings for each alternative
            alternateDescriptionFactory.AppendEncodedPolylineString(
                config.encode_geometry,
                reply.content
            );
        }
        reply.content.push_back("],");
        reply.content.push_back("\"alternative_instructions\":[");
        entered_restricted_area_count = 0;
        if(INT_MAX != raw_route_information.lengthOfAlternativePath) {
            reply.content.push_back("[");
            //Generate instructions for each alternative
            if(config.instructions) {
                BuildTextualDescription(
                    alternateDescriptionFactory,
                    reply,
                    raw_route_information.lengthOfAlternativePath,
                    facade,
                    alternative_path_segments
                );
            } else {
                BOOST_FOREACH(const SegmentInformation & segment, alternateDescriptionFactory.pathDescription) {
                	TurnInstruction current_instruction = segment.turnInstruction & TurnInstructions.InverseAccessRestrictionFlag;
                    entered_restricted_area_count += (current_instruction != segment.turnInstruction);
                }
            }
            reply.content.push_back("]");
        }
        reply.content.push_back("],");
        reply.content.push_back("\"alternative_summaries\":[");
        if(INT_MAX != raw_route_information.lengthOfAlternativePath) {
            //Generate route summary (length, duration) for each alternative
            alternateDescriptionFactory.BuildRouteSummary(alternateDescriptionFactory.entireLength, raw_route_information.lengthOfAlternativePath - ( entered_restricted_area_count*TurnInstructions.AccessRestrictionPenalty));
            reply.content.push_back("{");
            reply.content.push_back("\"total_distance\":");
            reply.content.push_back(alternateDescriptionFactory.summary.lengthString);
            reply.content.push_back(","
                    "\"total_time\":");
            reply.content.push_back(alternateDescriptionFactory.summary.durationString);
            reply.content.push_back(","
                    "\"start_point\":\"");
            reply.content.push_back(facade->GetEscapedNameForNameID(description_factory.summary.startName));
            reply.content.push_back("\","
                    "\"end_point\":\"");
            reply.content.push_back(facade->GetEscapedNameForNameID(description_factory.summary.destName));
            reply.content.push_back("\"");
            reply.content.push_back("}");
        }
        reply.content.push_back("],");

        //Get Names for both routes
        RouteNames routeNames;
        GetRouteNames(shortest_path_segments, alternative_path_segments, facade, routeNames);

        reply.content.push_back("\"route_name\":[\"");
        reply.content.push_back(routeNames.shortestPathName1);
        reply.content.push_back("\",\"");
        reply.content.push_back(routeNames.shortestPathName2);
        reply.content.push_back("\"],"
                "\"alternative_names\":[");
        reply.content.push_back("[\"");
        reply.content.push_back(routeNames.alternativePathName1);
        reply.content.push_back("\",\"");
        reply.content.push_back(routeNames.alternativePathName2);
        reply.content.push_back("\"]");
        reply.content.push_back("],");
        //list all viapoints so that the client may display it
        reply.content.push_back("\"via_points\":[");
        std::string tmp;
        if(config.geometry && INT_MAX != raw_route_information.lengthOfShortestPath) {
            for(unsigned i = 0; i < raw_route_information.segmentEndCoordinates.size(); ++i) {
                reply.content.push_back("[");
                if(raw_route_information.segmentEndCoordinates[i].startPhantom.location.isSet())
                    convertInternalReversedCoordinateToString(raw_route_information.segmentEndCoordinates[i].startPhantom.location, tmp);
                else
                    convertInternalReversedCoordinateToString(raw_route_information.rawViaNodeCoordinates[i], tmp);

                reply.content.push_back(tmp);
                reply.content.push_back("],");
            }
            reply.content.push_back("[");
            if(raw_route_information.segmentEndCoordinates.back().startPhantom.location.isSet())
                convertInternalReversedCoordinateToString(raw_route_information.segmentEndCoordinates.back().targetPhantom.location, tmp);
            else
                convertInternalReversedCoordinateToString(raw_route_information.rawViaNodeCoordinates.back(), tmp);
            reply.content.push_back(tmp);
            reply.content.push_back("]");
        }
        reply.content.push_back("],");
        reply.content.push_back("\"hint_data\": {");
        reply.content.push_back("\"checksum\":");
        intToString(raw_route_information.checkSum, tmp);
        reply.content.push_back(tmp);
        reply.content.push_back(", \"locations\": [");

        std::string hint;
        for(unsigned i = 0; i < raw_route_information.segmentEndCoordinates.size(); ++i) {
            reply.content.push_back("\"");
            EncodeObjectToBase64(raw_route_information.segmentEndCoordinates[i].startPhantom, hint);
            reply.content.push_back(hint);
            reply.content.push_back("\", ");
        }
        EncodeObjectToBase64(raw_route_information.segmentEndCoordinates.back().targetPhantom, hint);
        reply.content.push_back("\"");
        reply.content.push_back(hint);
        reply.content.push_back("\"]");
        reply.content.push_back("},");
        reply.content.push_back("\"transactionId\": \"OSRM Routing Engine JSON Descriptor (v0.3)\"");
        reply.content.push_back("}");
    }

    // construct routes names
    void GetRouteNames(
        std::vector<Segment> & shortest_path_segments,
        std::vector<Segment> & alternative_path_segments,
        const DataFacadeT * facade,
        RouteNames & routeNames
    ) {

        Segment shortestSegment1, shortestSegment2;
        Segment alternativeSegment1, alternativeSegment2;

        if(0 < shortest_path_segments.size()) {
            sort(shortest_path_segments.begin(), shortest_path_segments.end(), boost::bind(&Segment::length, _1) > boost::bind(&Segment::length, _2) );
            shortestSegment1 = shortest_path_segments[0];
            if(0 < alternative_path_segments.size()) {
                sort(alternative_path_segments.begin(), alternative_path_segments.end(), boost::bind(&Segment::length, _1) > boost::bind(&Segment::length, _2) );
                alternativeSegment1 = alternative_path_segments[0];
            }
            std::vector<Segment> shortestDifference(shortest_path_segments.size());
            std::vector<Segment> alternativeDifference(alternative_path_segments.size());
            std::set_difference(shortest_path_segments.begin(), shortest_path_segments.end(), alternative_path_segments.begin(), alternative_path_segments.end(), shortestDifference.begin(), boost::bind(&Segment::name_id, _1) < boost::bind(&Segment::name_id, _2) );
            int size_of_difference = shortestDifference.size();
            if(0 < size_of_difference ) {
                int i = 0;
                while( i < size_of_difference && shortestDifference[i].name_id == shortest_path_segments[0].name_id) {
                    ++i;
                }
                if(i < size_of_difference ) {
                    shortestSegment2 = shortestDifference[i];
                }
            }

            std::set_difference(alternative_path_segments.begin(), alternative_path_segments.end(), shortest_path_segments.begin(), shortest_path_segments.end(), alternativeDifference.begin(), boost::bind(&Segment::name_id, _1) < boost::bind(&Segment::name_id, _2) );
            size_of_difference = alternativeDifference.size();
            if(0 < size_of_difference ) {
                int i = 0;
                while( i < size_of_difference && alternativeDifference[i].name_id == alternative_path_segments[0].name_id) {
                    ++i;
                }
                if(i < size_of_difference ) {
                    alternativeSegment2 = alternativeDifference[i];
                }
            }
            if(shortestSegment1.position > shortestSegment2.position)
                std::swap(shortestSegment1, shortestSegment2);

            if(alternativeSegment1.position >  alternativeSegment2.position)
                std::swap(alternativeSegment1, alternativeSegment2);

            routeNames.shortestPathName1 = facade->GetEscapedNameForNameID(
                shortestSegment1.name_id
            );
            routeNames.shortestPathName2 = facade->GetEscapedNameForNameID(
                shortestSegment2.name_id
            );

            routeNames.alternativePathName1 = facade->GetEscapedNameForNameID(
                alternativeSegment1.name_id
            );
            routeNames.alternativePathName2 = facade->GetEscapedNameForNameID(
                alternativeSegment2.name_id
            );
        }
    }

    inline void WriteHeaderToOutput(std::vector<std::string> & output) {
        output.push_back(
            "{"
            "\"version\": 0.3,"
            "\"status\":"
        );
    }

    //TODO: reorder parameters
    inline void BuildTextualDescription(
        DescriptionFactory & description_factory,
        http::Reply & reply,
        const int route_length,
        const DataFacadeT * facade,
        std::vector<Segment> & route_segments_list
    ) {
        //Segment information has following format:
        //["instruction","streetname",length,position,time,"length","earth_direction",azimuth]
        //Example: ["Turn left","High Street",200,4,10,"200m","NE",22.5]
        //See also: http://developers.cloudmade.com/wiki/navengine/JSON_format
        unsigned prefixSumOfNecessarySegments = 0;
        roundAbout.leave_at_exit = 0;
        roundAbout.name_id = 0;
        std::string tmpDist, tmpLength, tmpDuration, tmpBearing, tmpInstruction;
        //Fetch data from Factory and generate a string from it.
        BOOST_FOREACH(const SegmentInformation & segment, description_factory.pathDescription) {
        	TurnInstruction current_instruction = segment.turnInstruction & TurnInstructions.InverseAccessRestrictionFlag;
            entered_restricted_area_count += (current_instruction != segment.turnInstruction);
            if(TurnInstructions.TurnIsNecessary( current_instruction) ) {
                if(TurnInstructions.EnterRoundAbout == current_instruction) {
                    roundAbout.name_id = segment.nameID;
                    roundAbout.start_index = prefixSumOfNecessarySegments;
                } else {
                    if(0 != prefixSumOfNecessarySegments){
                        reply.content.push_back(",");
                    }
                    reply.content.push_back("[\"");
                    if(TurnInstructions.LeaveRoundAbout == current_instruction) {
                        intToString(TurnInstructions.EnterRoundAbout, tmpInstruction);
                        reply.content.push_back(tmpInstruction);
                        reply.content.push_back("-");
                        intToString(roundAbout.leave_at_exit+1, tmpInstruction);
                        reply.content.push_back(tmpInstruction);
                        roundAbout.leave_at_exit = 0;
                    } else {
                        intToString(current_instruction, tmpInstruction);
                        reply.content.push_back(tmpInstruction);
                    }

                    reply.content.push_back("\",\"");
                    reply.content.push_back(facade->GetEscapedNameForNameID(segment.nameID));
                    reply.content.push_back("\",");
                    intToString(segment.length, tmpDist);
                    reply.content.push_back(tmpDist);
                    reply.content.push_back(",");
                    intToString(prefixSumOfNecessarySegments, tmpLength);
                    reply.content.push_back(tmpLength);
                    reply.content.push_back(",");
                    intToString(segment.duration/10, tmpDuration);
                    reply.content.push_back(tmpDuration);
                    reply.content.push_back(",\"");
                    intToString(segment.length, tmpLength);
                    reply.content.push_back(tmpLength);
                    reply.content.push_back("m\",\"");
                    reply.content.push_back(Azimuth::Get(segment.bearing));
                    reply.content.push_back("\",");
                    intToString(round(segment.bearing), tmpBearing);
                    reply.content.push_back(tmpBearing);
                    reply.content.push_back("]");

                    route_segments_list.push_back(
                        Segment(
                            segment.nameID,
                            segment.length,
                            route_segments_list.size()
                        )
                    );
                }
            } else if(TurnInstructions.StayOnRoundAbout == current_instruction) {
                ++roundAbout.leave_at_exit;
            }
            if(segment.necessary)
                ++prefixSumOfNecessarySegments;
        }
        if(INT_MAX != route_length) {
            reply.content.push_back(",[\"");
            intToString(TurnInstructions.ReachedYourDestination, tmpInstruction);
            reply.content.push_back(tmpInstruction);
            reply.content.push_back("\",\"");
            reply.content.push_back("\",");
            reply.content.push_back("0");
            reply.content.push_back(",");
            intToString(prefixSumOfNecessarySegments-1, tmpLength);
            reply.content.push_back(tmpLength);
            reply.content.push_back(",");
            reply.content.push_back("0");
            reply.content.push_back(",\"");
            reply.content.push_back("\",\"");
            reply.content.push_back(Azimuth::Get(0.0));
            reply.content.push_back("\",");
            reply.content.push_back("0.0");
            reply.content.push_back("]");
        }
    }

};
#endif /* JSON_DESCRIPTOR_H_ */
