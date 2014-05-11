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

#include <algorithm>

template <class DataFacadeT> class JSONDescriptor : public BaseDescriptor<DataFacadeT>
{
  private:
    // TODO: initalize in c'tor
    DataFacadeT *facade;
    DescriptorConfig config;
    DescriptionFactory description_factory;
    DescriptionFactory alternate_descriptionFactory;
    FixedPointCoordinate current;
    unsigned entered_restricted_area_count;
    struct RoundAbout
    {
        RoundAbout() : start_index(INT_MAX), name_id(INT_MAX), leave_at_exit(INT_MAX) {}
        int start_index;
        int name_id;
        int leave_at_exit;
    } round_about;

    struct Segment
    {
        Segment() : name_id(-1), length(-1), position(-1) {}
        Segment(int n, int l, int p) : name_id(n), length(l), position(p) {}
        int name_id;
        int length;
        int position;
    };
    std::vector<Segment> shortest_path_segments, alternative_path_segments;
    std::vector<unsigned> shortest_leg_end_indices, alternative_leg_end_indices;

    struct RouteNames
    {
        std::string shortest_path_name_1;
        std::string shortest_path_name_2;
        std::string alternative_path_name_1;
        std::string alternative_path_name_2;
    };

  public:
    JSONDescriptor() : facade(nullptr), entered_restricted_area_count(0)
    {
        shortest_leg_end_indices.emplace_back(0);
        alternative_leg_end_indices.emplace_back(0);
    }

    void SetConfig(const DescriptorConfig &c) { config = c; }

    unsigned DescribeLeg(const std::vector<PathData> route_leg, const PhantomNodes &leg_phantoms)
    {
        unsigned added_element_count = 0;
        // Get all the coordinates for the computed route
        FixedPointCoordinate current_coordinate;
        for (const PathData &path_data : route_leg)
        {
            current_coordinate = facade->GetCoordinateOfNode(path_data.node);
            description_factory.AppendSegment(current_coordinate, path_data);
            ++added_element_count;
        }
        ++added_element_count;
        BOOST_ASSERT((route_leg.size() + 1) == added_element_count);
        return added_element_count;
    }

    void Run(const RawRouteData &raw_route,
             const PhantomNodes &phantom_nodes,
             // TODO: move facade initalization to c'tor
             DataFacadeT *f,
             http::Reply &reply)
    {
        facade = f;
        reply.content.emplace_back("{\"status\":");

        if (INVALID_EDGE_WEIGHT == raw_route.shortest_path_length)
        {
            // We do not need to do much, if there is no route ;-)
            reply.content.emplace_back(
                "207,\"status_message\": \"Cannot find route between points\"}");
            return;
        }

        SimpleLogger().Write(logDEBUG) << "distance: " << raw_route.shortest_path_length;

        // check if first segment is non-zero
        std::string road_name =
            facade->GetEscapedNameForNameID(phantom_nodes.source_phantom.name_id);

        BOOST_ASSERT(raw_route.unpacked_path_segments.size() ==
                     raw_route.segment_end_coordinates.size());

        description_factory.SetStartSegment(phantom_nodes.source_phantom);
        reply.content.emplace_back("0,"
                                   "\"status_message\": \"Found route between points\",");

        // for each unpacked segment add the leg to the description
        for (unsigned i = 0; i < raw_route.unpacked_path_segments.size(); ++i)
        {
            const int added_segments = DescribeLeg(raw_route.unpacked_path_segments[i],
                                                   raw_route.segment_end_coordinates[i]);
            BOOST_ASSERT(0 < added_segments);
            shortest_leg_end_indices.emplace_back(added_segments + shortest_leg_end_indices.back());
        }
        description_factory.SetEndSegment(phantom_nodes.target_phantom);
        description_factory.Run(facade, config.zoom_level);

        reply.content.emplace_back("\"route_geometry\": ");
        if (config.geometry)
        {
            description_factory.AppendEncodedPolylineString(config.encode_geometry, reply.content);
        }
        else
        {
            reply.content.emplace_back("[]");
        }

        reply.content.emplace_back(",\"route_instructions\": [");
        if (config.instructions)
        {
            BuildTextualDescription(description_factory,
                                    reply,
                                    raw_route.shortest_path_length,
                                    facade,
                                    shortest_path_segments);
        }
        reply.content.emplace_back("],");
        description_factory.BuildRouteSummary(description_factory.entireLength,
                                              raw_route.shortest_path_length);

        reply.content.emplace_back("\"route_summary\":");
        reply.content.emplace_back("{");
        reply.content.emplace_back("\"total_distance\":");
        reply.content.emplace_back(description_factory.summary.lengthString);
        reply.content.emplace_back(","
                                   "\"total_time\":");
        reply.content.emplace_back(description_factory.summary.durationString);
        reply.content.emplace_back(","
                                   "\"start_point\":\"");
        reply.content.emplace_back(
            facade->GetEscapedNameForNameID(description_factory.summary.startName));
        reply.content.emplace_back("\","
                                   "\"end_point\":\"");
        reply.content.emplace_back(
            facade->GetEscapedNameForNameID(description_factory.summary.destName));
        reply.content.emplace_back("\"");
        reply.content.emplace_back("}");
        reply.content.emplace_back(",");

        // only one alternative route is computed at this time, so this is hardcoded
        if (raw_route.alternative_path_length != INVALID_EDGE_WEIGHT)
        {
            alternate_descriptionFactory.SetStartSegment(phantom_nodes.source_phantom);
            // Get all the coordinates for the computed route
            for (const PathData &path_data : raw_route.unpacked_alternative)
            {
                current = facade->GetCoordinateOfNode(path_data.node);
                alternate_descriptionFactory.AppendSegment(current, path_data);
            }
        }
        alternate_descriptionFactory.Run(facade, config.zoom_level);

        // //give an array of alternative routes
        reply.content.emplace_back("\"alternative_geometries\": [");
        if (config.geometry && INVALID_EDGE_WEIGHT != raw_route.alternative_path_length)
        {
            // Generate the linestrings for each alternative
            alternate_descriptionFactory.AppendEncodedPolylineString(config.encode_geometry,
                                                                     reply.content);
        }
        reply.content.emplace_back("],");
        reply.content.emplace_back("\"alternative_instructions\":[");
        if (INVALID_EDGE_WEIGHT != raw_route.alternative_path_length)
        {
            reply.content.emplace_back("[");
            // Generate instructions for each alternative
            if (config.instructions)
            {
                BuildTextualDescription(alternate_descriptionFactory,
                                        reply,
                                        raw_route.alternative_path_length,
                                        facade,
                                        alternative_path_segments);
            }
            reply.content.emplace_back("]");
        }
        reply.content.emplace_back("],");
        reply.content.emplace_back("\"alternative_summaries\":[");
        if (INVALID_EDGE_WEIGHT != raw_route.alternative_path_length)
        {
            // Generate route summary (length, duration) for each alternative
            alternate_descriptionFactory.BuildRouteSummary(
                alternate_descriptionFactory.entireLength, raw_route.alternative_path_length);
            reply.content.emplace_back("{");
            reply.content.emplace_back("\"total_distance\":");
            reply.content.emplace_back(alternate_descriptionFactory.summary.lengthString);
            reply.content.emplace_back(","
                                       "\"total_time\":");
            reply.content.emplace_back(alternate_descriptionFactory.summary.durationString);
            reply.content.emplace_back(","
                                       "\"start_point\":\"");
            reply.content.emplace_back(
                facade->GetEscapedNameForNameID(description_factory.summary.startName));
            reply.content.emplace_back("\","
                                       "\"end_point\":\"");
            reply.content.emplace_back(
                facade->GetEscapedNameForNameID(description_factory.summary.destName));
            reply.content.emplace_back("\"");
            reply.content.emplace_back("}");
        }
        reply.content.emplace_back("],");

        // //Get Names for both routes
        RouteNames routeNames;
        GetRouteNames(shortest_path_segments, alternative_path_segments, facade, routeNames);

        reply.content.emplace_back("\"route_name\":[\"");
        reply.content.emplace_back(routeNames.shortest_path_name_1);
        reply.content.emplace_back("\",\"");
        reply.content.emplace_back(routeNames.shortest_path_name_2);
        reply.content.emplace_back("\"],"
                                   "\"alternative_names\":[");
        reply.content.emplace_back("[\"");
        reply.content.emplace_back(routeNames.alternative_path_name_1);
        reply.content.emplace_back("\",\"");
        reply.content.emplace_back(routeNames.alternative_path_name_2);
        reply.content.emplace_back("\"]");
        reply.content.emplace_back("],");
        // list all viapoints so that the client may display it
        reply.content.emplace_back("\"via_points\":[");

        BOOST_ASSERT(!raw_route.segment_end_coordinates.empty());

        std::string tmp;
        FixedPointCoordinate::convertInternalReversedCoordinateToString(
            raw_route.segment_end_coordinates.front().source_phantom.location, tmp);
        reply.content.emplace_back("[");
        reply.content.emplace_back(tmp);
        reply.content.emplace_back("]");

        for (const PhantomNodes &nodes : raw_route.segment_end_coordinates)
        {
            tmp.clear();
            FixedPointCoordinate::convertInternalReversedCoordinateToString(
                nodes.target_phantom.location, tmp);
            reply.content.emplace_back(",[");
            reply.content.emplace_back(tmp);
            reply.content.emplace_back("]");
        }

        reply.content.emplace_back("],");
        reply.content.emplace_back("\"via_indices\":[");
        for (const unsigned index : shortest_leg_end_indices)
        {
            tmp.clear();
            intToString(index, tmp);
            reply.content.emplace_back(tmp);
            if (index != shortest_leg_end_indices.back())
            {
                reply.content.emplace_back(",");
            }
        }
        reply.content.emplace_back("],\"alternative_indices\":[");
        if (INVALID_EDGE_WEIGHT != raw_route.alternative_path_length)
        {
            reply.content.emplace_back("0,");
            tmp.clear();
            intToString(alternate_descriptionFactory.path_description.size(), tmp);
            reply.content.emplace_back(tmp);
        }

        reply.content.emplace_back("],");
        reply.content.emplace_back("\"hint_data\": {");
        reply.content.emplace_back("\"checksum\":");
        intToString(raw_route.check_sum, tmp);
        reply.content.emplace_back(tmp);
        reply.content.emplace_back(", \"locations\": [");

        std::string hint;
        for (unsigned i = 0; i < raw_route.segment_end_coordinates.size(); ++i)
        {
            reply.content.emplace_back("\"");
            EncodeObjectToBase64(raw_route.segment_end_coordinates[i].source_phantom, hint);
            reply.content.emplace_back(hint);
            reply.content.emplace_back("\", ");
        }
        EncodeObjectToBase64(raw_route.segment_end_coordinates.back().target_phantom, hint);
        reply.content.emplace_back("\"");
        reply.content.emplace_back(hint);
        reply.content.emplace_back("\"]");
        reply.content.emplace_back("}}");
    }

    // construct routes names
    void GetRouteNames(std::vector<Segment> &shortest_path_segments,
                       std::vector<Segment> &alternative_path_segments,
                       const DataFacadeT *facade,
                       RouteNames &routeNames)
    {
        Segment shortest_segment_1, shortest_segment_2;
        Segment alternativeSegment1, alternative_segment_2;

        auto length_comperator = [](Segment a, Segment b)
        { return a.length < b.length; };
        auto name_id_comperator = [](Segment a, Segment b)
        { return a.name_id < b.name_id; };

        if (!shortest_path_segments.empty())
        {
            std::sort(
                shortest_path_segments.begin(), shortest_path_segments.end(), length_comperator);
            shortest_segment_1 = shortest_path_segments[0];
            if (!alternative_path_segments.empty())
            {
                std::sort(alternative_path_segments.begin(),
                          alternative_path_segments.end(),
                          length_comperator);
                alternativeSegment1 = alternative_path_segments[0];
            }
            std::vector<Segment> shortestDifference(shortest_path_segments.size());
            std::vector<Segment> alternativeDifference(alternative_path_segments.size());
            std::set_difference(shortest_path_segments.begin(),
                                shortest_path_segments.end(),
                                alternative_path_segments.begin(),
                                alternative_path_segments.end(),
                                shortestDifference.begin(),
                                length_comperator);
            int size_of_difference = shortestDifference.size();
            if (size_of_difference)
            {
                int i = 0;
                while (i < size_of_difference &&
                       shortestDifference[i].name_id == shortest_path_segments[0].name_id)
                {
                    ++i;
                }
                if (i < size_of_difference)
                {
                    shortest_segment_2 = shortestDifference[i];
                }
            }

            std::set_difference(alternative_path_segments.begin(),
                                alternative_path_segments.end(),
                                shortest_path_segments.begin(),
                                shortest_path_segments.end(),
                                alternativeDifference.begin(),
                                name_id_comperator);
            size_of_difference = alternativeDifference.size();
            if (size_of_difference)
            {
                int i = 0;
                while (i < size_of_difference &&
                       alternativeDifference[i].name_id == alternative_path_segments[0].name_id)
                {
                    ++i;
                }
                if (i < size_of_difference)
                {
                    alternative_segment_2 = alternativeDifference[i];
                }
            }
            if (shortest_segment_1.position > shortest_segment_2.position)
                std::swap(shortest_segment_1, shortest_segment_2);

            if (alternativeSegment1.position > alternative_segment_2.position)
                std::swap(alternativeSegment1, alternative_segment_2);

            routeNames.shortest_path_name_1 =
                facade->GetEscapedNameForNameID(shortest_segment_1.name_id);
            routeNames.shortest_path_name_2 =
                facade->GetEscapedNameForNameID(shortest_segment_2.name_id);

            routeNames.alternative_path_name_1 =
                facade->GetEscapedNameForNameID(alternativeSegment1.name_id);
            routeNames.alternative_path_name_2 =
                facade->GetEscapedNameForNameID(alternative_segment_2.name_id);
        }
    }

    // TODO: reorder parameters
    inline void BuildTextualDescription(DescriptionFactory &description_factory,
                                        http::Reply &reply,
                                        const int route_length,
                                        const DataFacadeT *facade,
                                        std::vector<Segment> &route_segments_list)
    {
        // Segment information has following format:
        //["instruction","streetname",length,position,time,"length","earth_direction",azimuth]
        // Example: ["Turn left","High Street",200,4,10,"200m","NE",22.5]
        unsigned necessary_segments_running_index = 0;
        round_about.leave_at_exit = 0;
        round_about.name_id = 0;
        std::string temp_dist, temp_length, temp_duration, temp_bearing, temp_instruction;

        // Fetch data from Factory and generate a string from it.
        for (const SegmentInformation &segment : description_factory.path_description)
        {
            TurnInstruction current_instruction = segment.turn_instruction;
            entered_restricted_area_count += (current_instruction != segment.turn_instruction);
            if (TurnInstructionsClass::TurnIsNecessary(current_instruction))
            {
                if (TurnInstruction::EnterRoundAbout == current_instruction)
                {
                    round_about.name_id = segment.name_id;
                    round_about.start_index = necessary_segments_running_index;
                }
                else
                {
                    if (necessary_segments_running_index)
                    {
                        reply.content.emplace_back(",");
                    }
                    reply.content.emplace_back("[\"");
                    if (TurnInstruction::LeaveRoundAbout == current_instruction)
                    {
                        intToString(as_integer(TurnInstruction::EnterRoundAbout), temp_instruction);
                        reply.content.emplace_back(temp_instruction);
                        reply.content.emplace_back("-");
                        intToString(round_about.leave_at_exit + 1, temp_instruction);
                        reply.content.emplace_back(temp_instruction);
                        round_about.leave_at_exit = 0;
                    }
                    else
                    {
                        intToString(as_integer(current_instruction), temp_instruction);
                        reply.content.emplace_back(temp_instruction);
                    }

                    reply.content.emplace_back("\",\"");
                    reply.content.emplace_back(facade->GetEscapedNameForNameID(segment.name_id));
                    reply.content.emplace_back("\",");
                    intToString(segment.length, temp_dist);
                    reply.content.emplace_back(temp_dist);
                    reply.content.emplace_back(",");
                    intToString(necessary_segments_running_index, temp_length);
                    reply.content.emplace_back(temp_length);
                    reply.content.emplace_back(",");
                    intToString(round(segment.duration / 10.), temp_duration);
                    reply.content.emplace_back(temp_duration);
                    reply.content.emplace_back(",\"");
                    intToString(segment.length, temp_length);
                    reply.content.emplace_back(temp_length);
                    reply.content.emplace_back("m\",\"");
                    int bearing_value = round(segment.bearing / 10.);
                    reply.content.emplace_back(Azimuth::Get(bearing_value));
                    reply.content.emplace_back("\",");
                    intToString(bearing_value, temp_bearing);
                    reply.content.emplace_back(temp_bearing);
                    reply.content.emplace_back("]");

                    route_segments_list.emplace_back(
                        Segment(segment.name_id, segment.length, route_segments_list.size()));
                }
            }
            else if (TurnInstruction::StayOnRoundAbout == current_instruction)
            {
                ++round_about.leave_at_exit;
            }
            if (segment.necessary)
            {
                ++necessary_segments_running_index;
            }
        }
        if (INVALID_EDGE_WEIGHT != route_length)
        {
            reply.content.emplace_back(",[\"");
            intToString(as_integer(TurnInstruction::ReachedYourDestination), temp_instruction);
            reply.content.emplace_back(temp_instruction);
            reply.content.emplace_back("\",\"");
            reply.content.emplace_back("\",");
            reply.content.emplace_back("0");
            reply.content.emplace_back(",");
            intToString(necessary_segments_running_index - 1, temp_length);
            reply.content.emplace_back(temp_length);
            reply.content.emplace_back(",");
            reply.content.emplace_back("0");
            reply.content.emplace_back(",\"");
            reply.content.emplace_back("\",\"");
            reply.content.emplace_back(Azimuth::Get(0.0));
            reply.content.emplace_back("\",");
            reply.content.emplace_back("0.0");
            reply.content.emplace_back("]");
        }
    }
};

#endif /* JSON_DESCRIPTOR_H_ */
