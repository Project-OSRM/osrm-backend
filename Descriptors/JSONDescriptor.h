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
    // TODO: initalize in c'tor
    DataFacadeT * facade;
    DescriptorConfig config;
    DescriptionFactory description_factory;
    DescriptionFactory alternate_descriptionFactory;
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
    } round_about;

    struct Segment {
        Segment() : name_id(-1), length(-1), position(-1) {}
        Segment(int n, int l, int p) : name_id(n), length(l), position(p) {}
        int name_id;
        int length;
        int position;
    };
    std::vector<Segment> shortest_path_segments, alternative_path_segments;
    std::vector<unsigned> shortest_leg_end_indices, alternative_leg_end_indices;

    struct RouteNames {
        std::string shortestPathName1;
        std::string shortestPathName2;
        std::string alternativePathName1;
        std::string alternativePathName2;
    };

public:
    JSONDescriptor() :
        facade(NULL),
        entered_restricted_area_count(0)
    {
        shortest_leg_end_indices.push_back(0);
        alternative_leg_end_indices.push_back(0);
    }

    void SetConfig(const DescriptorConfig & c) { config = c; }

    int DescribeLeg(
        const std::vector<PathData> & route_leg,
        const PhantomNodes & leg_phantoms
    ) {
        int added_element_count = 0;
        //Get all the coordinates for the computed route
        FixedPointCoordinate current_coordinate;
        BOOST_FOREACH(const PathData & path_data, route_leg) {
            current_coordinate = facade->GetCoordinateOfNode(path_data.node);
            description_factory.AppendSegment(current_coordinate, path_data);
            ++added_element_count;
        }
        // description_factory.SetEndSegment( leg_phantoms.target_phantom );
        ++added_element_count;
        BOOST_ASSERT( (int)(route_leg.size() + 1) == added_element_count );
        return added_element_count;
    }

    void Run(
        const RawRouteData & raw_route,
        const PhantomNodes & phantom_nodes,
        // TODO: move facade initalization to c'tor
        DataFacadeT * f,
        http::Reply & reply
    ) {
        facade = f;
        reply.content.push_back(
            "{\"status\":"
        );

        if (INVALID_EDGE_WEIGHT == raw_route.lengthOfShortestPath)
        {
            //We do not need to do much, if there is no route ;-)
            reply.content.push_back(
                "207,\"status_message\": \"Cannot find route between points\"}"
            );
            return;
        }

        //check if first segment is non-zero
        std::string road_name;
        int source_duration = phantom_nodes.source_phantom.GetForwardWeightPlusOffset();
        // SimpleLogger().Write(logDEBUG) << "-> source_traversed_in_reverse: " << (raw_route.source_traversed_in_reverse ? "y" : "n");
        if (!raw_route.source_traversed_in_reverse)
        {
            source_duration = phantom_nodes.source_phantom.GetReverseWeightPlusOffset();
        }
        BOOST_ASSERT(source_duration >= 0);
        road_name = facade->GetEscapedNameForNameID(phantom_nodes.source_phantom.name_id);

        // for each unpacked segment add the leg to the description
        BOOST_ASSERT( raw_route.unpacked_path_segments.size() == raw_route.segmentEndCoordinates.size() );

        for (unsigned i = 0; i < raw_route.unpacked_path_segments.size(); ++i)
        {
            const std::vector<PathData> & leg_path = raw_route.unpacked_path_segments[i];
            FixedPointCoordinate current_coordinate;
            BOOST_FOREACH(const PathData & path_data, leg_path)
            {
                current_coordinate = facade->GetCoordinateOfNode(path_data.node);
                road_name = facade->GetEscapedNameForNameID(path_data.name_id);
            }
        }

        //check if last segment is non-zero
        road_name = facade->GetEscapedNameForNameID(phantom_nodes.target_phantom.name_id);

        int target_duration = phantom_nodes.target_phantom.GetForwardWeightPlusOffset();
        if (raw_route.target_traversed_in_reverse)
        {
            target_duration = phantom_nodes.target_phantom.GetReverseWeightPlusOffset();
        }
        BOOST_ASSERT(target_duration >= 0);

        // if (target_duration > 0)
        // {
        //     SimpleLogger().Write(logDEBUG) << "adding target \"" << road_name << "\" at " << phantom_nodes.target_phantom.location << ", duration: " << target_duration;
        // }
        // else
        // {
        //     SimpleLogger().Write(logDEBUG) << "ignoring target \"" << road_name << "\"";
        // }
        // SimpleLogger().Write(logDEBUG) << "-> target_traversed_in_reverse: " << (raw_route.target_traversed_in_reverse ? "y" : "n");

        //END OF TODO

        description_factory.SetStartSegment(phantom_nodes.source_phantom, raw_route.source_traversed_in_reverse);
        reply.content.push_back("0,"
                "\"status_message\": \"Found route between points\",");

        BOOST_ASSERT( raw_route.unpacked_path_segments.size() == raw_route.segmentEndCoordinates.size() );
        for (unsigned i = 0; i < raw_route.unpacked_path_segments.size(); ++i)
        {
            const int added_segments = DescribeLeg(
                raw_route.unpacked_path_segments[i],
                raw_route.segmentEndCoordinates[i]
            );
            BOOST_ASSERT( 0 < added_segments );
            shortest_leg_end_indices.push_back(
                added_segments + shortest_leg_end_indices.back()
            );
        }
        description_factory.SetEndSegment(phantom_nodes.target_phantom, raw_route.target_traversed_in_reverse);
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

        reply.content.push_back(",\"route_instructions\": [");
        if(config.instructions) {
            BuildTextualDescription(
                description_factory,
                reply,
                raw_route.lengthOfShortestPath,
                facade,
                shortest_path_segments
            );
        }
        reply.content.push_back("],");
        description_factory.BuildRouteSummary(
            description_factory.entireLength,
            raw_route.lengthOfShortestPath
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


        if(raw_route.lengthOfAlternativePath != INVALID_EDGE_WEIGHT)
        {
            alternate_descriptionFactory.SetStartSegment(phantom_nodes.source_phantom, raw_route.alt_source_traversed_in_reverse);
            //Get all the coordinates for the computed route
            BOOST_FOREACH(const PathData & path_data, raw_route.unpacked_alternative) {
                current = facade->GetCoordinateOfNode(path_data.node);
                alternate_descriptionFactory.AppendSegment(current, path_data );
            }
            alternate_descriptionFactory.SetEndSegment(phantom_nodes.target_phantom, raw_route.alt_target_traversed_in_reverse);
        }
        alternate_descriptionFactory.Run(facade, config.zoom_level);

        // //give an array of alternative routes
        reply.content.push_back("\"alternative_geometries\": [");
        if(config.geometry && INT_MAX != raw_route.lengthOfAlternativePath) {
            //Generate the linestrings for each alternative
            alternate_descriptionFactory.AppendEncodedPolylineString(
                config.encode_geometry,
                reply.content
            );
        }
        reply.content.push_back("],");
        reply.content.push_back("\"alternative_instructions\":[");
        if(INT_MAX != raw_route.lengthOfAlternativePath) {
            reply.content.push_back("[");
            //Generate instructions for each alternative
            if(config.instructions) {
                BuildTextualDescription(
                    alternate_descriptionFactory,
                    reply,
                    raw_route.lengthOfAlternativePath,
                    facade,
                    alternative_path_segments
                );
            }
            reply.content.push_back("]");
        }
        reply.content.push_back("],");
        reply.content.push_back("\"alternative_summaries\":[");
        if(INT_MAX != raw_route.lengthOfAlternativePath) {
            //Generate route summary (length, duration) for each alternative
            alternate_descriptionFactory.BuildRouteSummary(
                alternate_descriptionFactory.entireLength,
                raw_route.lengthOfAlternativePath
            );
            reply.content.push_back("{");
            reply.content.push_back("\"total_distance\":");
            reply.content.push_back(
                alternate_descriptionFactory.summary.lengthString
            );
            reply.content.push_back(","
                    "\"total_time\":");
            reply.content.push_back(
                alternate_descriptionFactory.summary.durationString
            );
            reply.content.push_back(","
                    "\"start_point\":\"");
            reply.content.push_back(
                facade->GetEscapedNameForNameID(
                    description_factory.summary.startName
                )
            );
            reply.content.push_back("\","
                    "\"end_point\":\"");
            reply.content.push_back(facade->GetEscapedNameForNameID(description_factory.summary.destName));
            reply.content.push_back("\"");
            reply.content.push_back("}");
        }
        reply.content.push_back("],");

        // //Get Names for both routes
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

        BOOST_ASSERT( !raw_route.segmentEndCoordinates.empty() );

        std::string tmp;
        FixedPointCoordinate::convertInternalReversedCoordinateToString(
            raw_route.segmentEndCoordinates.front().source_phantom.location,
            tmp
        );
        reply.content.push_back("[");
        reply.content.push_back(tmp);
        reply.content.push_back("]");

        BOOST_FOREACH(const PhantomNodes & nodes, raw_route.segmentEndCoordinates) {
            tmp.clear();
            FixedPointCoordinate::convertInternalReversedCoordinateToString(
                nodes.target_phantom.location,
                tmp
            );
            reply.content.push_back(",[");
            reply.content.push_back(tmp);
            reply.content.push_back("]");
        }

        reply.content.push_back("],");
        reply.content.push_back("\"via_indices\":[");
        BOOST_FOREACH(const unsigned index, shortest_leg_end_indices) {
            tmp.clear();
            intToString(index, tmp);
            reply.content.push_back(tmp);
            if( index != shortest_leg_end_indices.back() ) {
                reply.content.push_back(",");
            }
        }
        reply.content.push_back("],\"alternative_indices\":[");
        if(INT_MAX != raw_route.lengthOfAlternativePath) {
            reply.content.push_back("0,");
            tmp.clear();
            intToString(alternate_descriptionFactory.pathDescription.size(), tmp);
            reply.content.push_back(tmp);
        }

        reply.content.push_back("],");
        reply.content.push_back("\"hint_data\": {");
        reply.content.push_back("\"checksum\":");
        intToString(raw_route.checkSum, tmp);
        reply.content.push_back(tmp);
        reply.content.push_back(", \"locations\": [");

        std::string hint;
        for(unsigned i = 0; i < raw_route.segmentEndCoordinates.size(); ++i) {
            reply.content.push_back("\"");
            EncodeObjectToBase64(raw_route.segmentEndCoordinates[i].source_phantom, hint);
            reply.content.push_back(hint);
            reply.content.push_back("\", ");
        }
        EncodeObjectToBase64(raw_route.segmentEndCoordinates.back().target_phantom, hint);
        reply.content.push_back("\"");
        reply.content.push_back(hint);
        reply.content.push_back("\"]");
        reply.content.push_back("}}");
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
        unsigned necessary_segments_running_index = 0;
        round_about.leave_at_exit = 0;
        round_about.name_id = 0;
        std::string temp_dist, temp_length, temp_duration, temp_bearing, temp_instruction;

        //Fetch data from Factory and generate a string from it.
        BOOST_FOREACH(const SegmentInformation & segment, description_factory.pathDescription) {
        	TurnInstruction current_instruction = segment.turn_instruction & TurnInstructionsClass::InverseAccessRestrictionFlag;
            entered_restricted_area_count += (current_instruction != segment.turn_instruction);
            if (TurnInstructionsClass::TurnIsNecessary( current_instruction) )
            {
                if (TurnInstructionsClass::EnterRoundAbout == current_instruction)
                {
                    round_about.name_id = segment.name_id;
                    round_about.start_index = necessary_segments_running_index;
                }
                else
                {
                    if (0 != necessary_segments_running_index)
                    {
                        reply.content.push_back(",");
                    }
                    reply.content.push_back("[\"");
                    if(TurnInstructionsClass::LeaveRoundAbout == current_instruction) {
                        intToString(TurnInstructionsClass::EnterRoundAbout, temp_instruction);
                        reply.content.push_back(temp_instruction);
                        reply.content.push_back("-");
                        intToString(round_about.leave_at_exit+1, temp_instruction);
                        reply.content.push_back(temp_instruction);
                        round_about.leave_at_exit = 0;
                    } else {
                        intToString(current_instruction, temp_instruction);
                        reply.content.push_back(temp_instruction);
                    }

                    reply.content.push_back("\",\"");
                    reply.content.push_back(facade->GetEscapedNameForNameID(segment.name_id));
                    reply.content.push_back("\",");
                    intToString(segment.length, temp_dist);
                    reply.content.push_back(temp_dist);
                    reply.content.push_back(",");
                    intToString(necessary_segments_running_index, temp_length);
                    reply.content.push_back(temp_length);
                    reply.content.push_back(",");
                    intToString(segment.duration/10, temp_duration);
                    reply.content.push_back(temp_duration);
                    reply.content.push_back(",\"");
                    intToString(segment.length, temp_length);
                    reply.content.push_back(temp_length);
                    reply.content.push_back("m\",\"");
                    double bearing_value = round(segment.bearing/10.);
                    reply.content.push_back(Azimuth::Get(bearing_value));
                    reply.content.push_back("\",");
                    intToString(bearing_value, temp_bearing);
                    reply.content.push_back(temp_bearing);
                    reply.content.push_back("]");

                    route_segments_list.push_back(
                        Segment(
                            segment.name_id,
                            segment.length,
                            route_segments_list.size()
                        )
                    );
                }
            } else if(TurnInstructionsClass::StayOnRoundAbout == current_instruction) {
                ++round_about.leave_at_exit;
            }
            if(segment.necessary)
                ++necessary_segments_running_index;
        }
        if(INT_MAX != route_length) {
            reply.content.push_back(",[\"");
            intToString(TurnInstructionsClass::ReachedYourDestination, temp_instruction);
            reply.content.push_back(temp_instruction);
            reply.content.push_back("\",\"");
            reply.content.push_back("\",");
            reply.content.push_back("0");
            reply.content.push_back(",");
            intToString(necessary_segments_running_index-1, temp_length);
            reply.content.push_back(temp_length);
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
