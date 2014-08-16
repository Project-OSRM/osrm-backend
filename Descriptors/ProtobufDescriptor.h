/*

Copyright (c) 2014, Project OSRM, Kirill Zhdanovich
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


#ifndef PBF_DESCRIPTOR_H
#define PBF_DESCRIPTOR_H

#include "BaseDescriptor.h"
#include "DescriptionFactory.h"
#include "response.pb.h"

template <class DataFacadeT> class PBFDescriptor : public BaseDescriptor<DataFacadeT>
{
  private:
    DataFacadeT *facade;
    DescriptorConfig config;
    DescriptionFactory description_factory, alternate_description_factory;
    FixedPointCoordinate current;
    unsigned entered_restricted_area_count;
    struct RoundAbout
    {
        RoundAbout() : start_index(INT_MAX), name_id(INVALID_NAMEID), leave_at_exit(INT_MAX) {}
        int start_index;
        unsigned name_id;
        int leave_at_exit;
    } round_about;

    struct Segment
    {
        Segment() : name_id(INVALID_NAMEID), length(-1), position(0) {}
        Segment(unsigned n, int l, unsigned p) : name_id(n), length(l), position(p) {}
        unsigned name_id;
        int length;
        unsigned position;
    };
    std::vector<Segment> shortest_path_segments, alternative_path_segments;
    ExtractRouteNames<DataFacadeT, Segment> GenerateRouteNames;

    inline void AddInstructionToRoute(protobufResponse::Route &route,
                                      const std::string &id,
                                      const std::string &streetName,
                                      const int &length,
                                      const unsigned &position,
                                      const int &time,
                                      const std::string &lengthStr,
                                      const std::string &earthDirection,
                                      const int &azimuth)
    {
        protobufResponse::RouteInstructions routeInstructions;
        routeInstructions.set_instruction_id(id);
        routeInstructions.set_street_name(streetName);
        routeInstructions.set_length(length);
        routeInstructions.set_position(position);
        routeInstructions.set_time(time);
        routeInstructions.set_length_str(lengthStr);
        routeInstructions.set_earth_direction(earthDirection);
        routeInstructions.set_azimuth(azimuth);
        route.add_route_instructions()->CopyFrom(routeInstructions);

    }

    inline void BuildTextualDescription(const int route_length,
                                        DescriptionFactory &description_factory,
                                        std::vector<Segment> &route_segments_list,
                                        protobufResponse::Route &route
                                        )
    {
        // Segment information has following format:
        //["instruction id","streetname",length,position,time,"length","earth_direction",azimuth]
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
                    std::string current_turn_instruction;
                    if (TurnInstruction::LeaveRoundAbout == current_instruction)
                    {
                        temp_instruction =
                            IntToString(as_integer(TurnInstruction::EnterRoundAbout));
                        current_turn_instruction += temp_instruction;
                        current_turn_instruction += "-";
                        temp_instruction = IntToString(round_about.leave_at_exit + 1);
                        current_turn_instruction += temp_instruction;
                        round_about.leave_at_exit = 0;
                    }
                    else
                    {
                        temp_instruction = IntToString(as_integer(current_instruction));
                        current_turn_instruction += temp_instruction;
                    }

                    const double bearing_value = (segment.bearing / 10.);
                    AddInstructionToRoute(route,
                                          current_turn_instruction,
                                          facade->GetEscapedNameForNameID(segment.name_id),
                                          std::round(segment.length),
                                          necessary_segments_running_index,
                                          round(segment.duration / 10),
                                          UintToString(static_cast<int>(segment.length)) + "m",
                                          Azimuth::Get(bearing_value),
                                          round(bearing_value));
                    route_segments_list.emplace_back(
                        segment.name_id, static_cast<int>(segment.length), static_cast<unsigned>(route_segments_list.size()));
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

        temp_instruction = IntToString(as_integer(TurnInstruction::ReachedYourDestination));
        AddInstructionToRoute(route,
                              temp_instruction, "", 0,
                              necessary_segments_running_index - 1,
                              0, "0m", Azimuth::Get(0.0),0.);
    }

  public:
    PBFDescriptor(DataFacadeT *facade) : facade(facade) {}

    void SetConfig(const DescriptorConfig &c) { config = c; }

    unsigned DescribeLeg(const std::vector<PathData> route_leg,
                         const PhantomNodes &leg_phantoms,
                         const bool target_traversed_in_reverse,
                         const bool is_via_leg)
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
        description_factory.SetEndSegment(leg_phantoms.target_phantom, target_traversed_in_reverse, is_via_leg);
        ++added_element_count;
        BOOST_ASSERT((route_leg.size() + 1) == added_element_count);
        return added_element_count;
    }

    void Run(const RawRouteData &raw_route, http::Reply &reply)
    {
        protobufResponse::Response response;
        std::string output;

        if (INVALID_EDGE_WEIGHT == raw_route.shortest_path_length)
        {
            // We do not need to do much, if there is no route ;-)
            response.set_status(207);
            response.set_status_message("Cannot find route between points");
            response.SerializeToString(&output);
            reply.content.insert(reply.content.end(), output.begin(), output.end());
            return;
        }

        std::string road_name = facade->GetEscapedNameForNameID(
            raw_route.segment_end_coordinates.front().source_phantom.name_id);

        BOOST_ASSERT(raw_route.unpacked_path_segments.size() ==
                     raw_route.segment_end_coordinates.size());

        description_factory.SetStartSegment(
            raw_route.segment_end_coordinates.front().source_phantom,
            raw_route.source_traversed_in_reverse.front());
        response.set_status(0);
        response.set_status_message("Found route between points");

        // for each unpacked segment add the leg to the description
        for (const auto i : osrm::irange<std::size_t>(0, raw_route.unpacked_path_segments.size()))
        {
#ifndef NDEBUG
            const int added_segments =
#endif
            DescribeLeg(raw_route.unpacked_path_segments[i],
                        raw_route.segment_end_coordinates[i],
                        raw_route.target_traversed_in_reverse[i],
                        raw_route.is_via_leg(i));
            BOOST_ASSERT(0 < added_segments);
        }

        protobufResponse::Route mainRoute;
        description_factory.Run(facade, config.zoom_level);
        if (config.geometry)
        {
            std::string route_geometry;
            description_factory.AppendEncodedPolylineStringEncoded(route_geometry);
            mainRoute.set_route_geometry(route_geometry);
        }
        if (config.instructions)
        {
            BuildTextualDescription(raw_route.shortest_path_length,
                                    description_factory,
                                    shortest_path_segments,
                                    mainRoute);
        }

        //route_instructions

        description_factory.BuildRouteSummary(description_factory.entireLength,
                                              raw_route.shortest_path_length);

        protobufResponse::RouteSummary routeSummary;

        routeSummary.set_total_distance(description_factory.summary.distance);
        routeSummary.set_total_time(description_factory.summary.duration);
        routeSummary.set_start_point(facade->GetEscapedNameForNameID(description_factory.summary.source_name_id));
        routeSummary.set_end_point(facade->GetEscapedNameForNameID(description_factory.summary.target_name_id));
        mainRoute.mutable_route_summary()->CopyFrom(routeSummary);

        BOOST_ASSERT(!raw_route.segment_end_coordinates.empty());

        protobufResponse::Point point;
        point.set_lat(raw_route.segment_end_coordinates.front().source_phantom.location.lat /
                                             COORDINATE_PRECISION);
        point.set_lon(raw_route.segment_end_coordinates.front().source_phantom.location.lon /
                                             COORDINATE_PRECISION);
        mainRoute.add_via_points()->CopyFrom(point);


        for (const PhantomNodes &nodes : raw_route.segment_end_coordinates)
        {
            point.set_lat(nodes.target_phantom.location.lat /
                                             COORDINATE_PRECISION);
            point.set_lon(nodes.target_phantom.location.lon /
                                             COORDINATE_PRECISION);
            mainRoute.add_via_points()->CopyFrom(point);
        }

        std::vector<unsigned> const &shortest_leg_end_indices = description_factory.GetViaIndices();
        for (unsigned v : shortest_leg_end_indices)
        {
            mainRoute.add_via_indices(v);
        }

        RouteNames route_names =
            GenerateRouteNames(shortest_path_segments, alternative_path_segments, facade);

        if (INVALID_EDGE_WEIGHT != raw_route.alternative_path_length)
        {
            protobufResponse::Route alternativeRoute;
            BOOST_ASSERT(!raw_route.alt_source_traversed_in_reverse.empty());
            alternate_description_factory.SetStartSegment(
                raw_route.segment_end_coordinates.front().source_phantom,
                raw_route.alt_source_traversed_in_reverse.front());
            // Get all the coordinates for the computed route
            for (const PathData &path_data : raw_route.unpacked_alternative)
            {
                current = facade->GetCoordinateOfNode(path_data.node);
                alternate_description_factory.AppendSegment(current, path_data);
            }
            alternate_description_factory.SetEndSegment(raw_route.segment_end_coordinates.back().target_phantom, raw_route.alt_source_traversed_in_reverse.back());
            alternate_description_factory.Run(facade, config.zoom_level);

            if (config.geometry)
            {
                std::string alternateGeometry;
                alternate_description_factory.AppendEncodedPolylineStringEncoded(alternateGeometry);
                alternativeRoute.set_route_geometry(alternateGeometry);
            }
            // Generate instructions for each alternative (simulated here)
            if (config.instructions)
            {
                BuildTextualDescription(raw_route.alternative_path_length,
                                        alternate_description_factory,
                                        alternative_path_segments,
                                        alternativeRoute);
            }

            alternate_description_factory.BuildRouteSummary(
                alternate_description_factory.entireLength, raw_route.alternative_path_length);

            protobufResponse::RouteSummary alternativeRouteSummary;

            alternativeRouteSummary.set_total_distance(alternate_description_factory.summary.distance);
            alternativeRouteSummary.set_total_time(alternate_description_factory.summary.duration);
            alternativeRouteSummary.set_start_point(facade->GetEscapedNameForNameID(
                alternate_description_factory.summary.source_name_id));
            alternativeRouteSummary.set_end_point(facade->GetEscapedNameForNameID(
                alternate_description_factory.summary.target_name_id));
            alternativeRoute.mutable_route_summary()->CopyFrom(routeSummary);

            std::vector<unsigned> const &alternate_leg_end_indices =
                alternate_description_factory.GetViaIndices();
            for (unsigned v : alternate_leg_end_indices)
            {
                alternativeRoute.add_via_indices(v);
            }

            alternativeRoute.add_route_name(route_names.alternative_path_name_1);
            alternativeRoute.add_route_name(route_names.alternative_path_name_2);

            response.mutable_alternative_route()->CopyFrom(mainRoute);
        }

        mainRoute.add_route_name(route_names.shortest_path_name_1);
        mainRoute.add_route_name(route_names.shortest_path_name_2);

        protobufResponse::Hint hint;
        hint.set_check_sum(raw_route.check_sum);
        std::string res;
        for (const auto i : osrm::irange<std::size_t>(0, raw_route.segment_end_coordinates.size()))
        {
            EncodeObjectToBase64(raw_route.segment_end_coordinates[i].source_phantom, res);
            hint.add_location(res);
        }
        EncodeObjectToBase64(raw_route.segment_end_coordinates.back().target_phantom, res);
        hint.add_location(res);

        response.mutable_hint()->CopyFrom(hint);
        response.mutable_main_route()->CopyFrom(mainRoute);

        std::cout << response.DebugString() << std::endl;

        response.SerializeToString(&output);
        reply.content.insert(reply.content.end(), output.begin(), output.end());
    }
};
#endif // PBF_DESCRIPTOR_H

