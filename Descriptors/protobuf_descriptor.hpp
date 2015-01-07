/*

Copyright (c) 2015, Project OSRM, Kirill Zhdanovich
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

#ifndef PROTOBUF_DESCRIPTOR_HPP
#define PROTOBUF_DESCRIPTOR_HPP

#include "descriptor_base.hpp"
#include "response.pb.h"

#include "../Util/simple_logger.hpp"

template <class DataFacadeT> class PBFDescriptor : public BaseDescriptor<DataFacadeT>
{
  private:
    typedef BaseDescriptor<DataFacadeT> super;

    inline void AddInstructionToRoute(protobufResponse::Route &route,
                                      const typename super::Instruction &i)
    {
        protobufResponse::RouteInstructions route_instructions;
        route_instructions.set_instruction_id(i.instruction_id);
        route_instructions.set_street_name(i.street_name);
        route_instructions.set_length(i.length);
        route_instructions.set_position(i.position);
        route_instructions.set_time(i.time);
        route_instructions.set_length_str(i.length_string);
        route_instructions.set_earth_direction(i.bearing);
        route_instructions.set_azimuth(i.azimuth);
        route_instructions.set_travel_mode(i.travel_mode);
        route.add_route_instructions()->CopyFrom(route_instructions);
    }

    inline void BuildTextualDescription(const int route_length,
                                        DescriptionFactory &description_factory,
                                        std::vector<typename super::Segment> &route_segments_list,
                                        protobufResponse::Route &route)
    {
        std::vector<typename super::Instruction> instructions;
        super::BuildTextualDescription(
            description_factory, instructions, route_length, route_segments_list);

        for (const auto &i : instructions)
        {
            AddInstructionToRoute(route, i);
        }
    }

  public:
    PBFDescriptor(DataFacadeT *facade) : super(facade) {}

    void Run(const InternalRouteResult &raw_route, JSON::Object &json_result) final
    {
        std::vector<char> result_vector;
        protobufResponse::Response response;
        std::string output;

        if (INVALID_EDGE_WEIGHT == raw_route.shortest_path_length)
        {
            // We do not need to do much, if there is no route ;-)
            response.set_status(207);
            response.set_status_message("Cannot find route between points");
            response.SerializeToString(&output);
            result_vector.insert(result_vector.end(), output.begin(), output.end());
            return;
        }

        std::string road_name = super::facade->GetEscapedNameForNameID(
            raw_route.segment_end_coordinates.front().source_phantom.name_id);

        BOOST_ASSERT(raw_route.unpacked_path_segments.size() ==
                     raw_route.segment_end_coordinates.size());

        super::description_factory.SetStartSegment(
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
                super::DescribeLeg(raw_route.unpacked_path_segments[i],
                                   raw_route.segment_end_coordinates[i],
                                   raw_route.target_traversed_in_reverse[i],
                                   raw_route.is_via_leg(i));
            BOOST_ASSERT(0 < added_segments);
        }

        protobufResponse::Route main_route;
        super::description_factory.Run(super::config.zoom_level);
        if (super::config.geometry)
        {
            std::string route_geometry;
            route_geometry = super::description_factory.AppendEncodedPolylineStringEncoded();
            main_route.set_route_geometry(route_geometry);
        }
        if (super::config.instructions)
        {
            BuildTextualDescription(raw_route.shortest_path_length,
                                    super::description_factory,
                                    super::shortest_path_segments,
                                    main_route);
        }

        super::description_factory.BuildRouteSummary(super::description_factory.get_entire_length(),
                                                     raw_route.shortest_path_length);

        protobufResponse::RouteSummary route_summary;

        route_summary.set_total_distance(super::description_factory.summary.distance);
        route_summary.set_total_time(super::description_factory.summary.duration);
        route_summary.set_start_point(super::facade->GetEscapedNameForNameID(
            super::description_factory.summary.source_name_id));
        route_summary.set_end_point(super::facade->GetEscapedNameForNameID(
            super::description_factory.summary.target_name_id));
        main_route.mutable_route_summary()->CopyFrom(route_summary);

        BOOST_ASSERT(!raw_route.segment_end_coordinates.empty());

        protobufResponse::Point point;
        point.set_lat(raw_route.segment_end_coordinates.front().source_phantom.location.lat /
                      COORDINATE_PRECISION);
        point.set_lon(raw_route.segment_end_coordinates.front().source_phantom.location.lon /
                      COORDINATE_PRECISION);
        main_route.add_via_points()->CopyFrom(point);

        for (const PhantomNodes &nodes : raw_route.segment_end_coordinates)
        {
            point.set_lat(nodes.target_phantom.location.lat / COORDINATE_PRECISION);
            point.set_lon(nodes.target_phantom.location.lon / COORDINATE_PRECISION);
            main_route.add_via_points()->CopyFrom(point);
        }

        const std::vector<unsigned> &shortest_leg_end_indices =
            super::description_factory.GetViaIndices();
        for (const auto v : shortest_leg_end_indices)
        {
            main_route.add_via_indices(v);
        }

        RouteNames route_names = super::GenerateRouteNames(
            super::shortest_path_segments, super::alternative_path_segments, super::facade);

        if (INVALID_EDGE_WEIGHT != raw_route.alternative_path_length)
        {
            protobufResponse::Route alternative_route;
            BOOST_ASSERT(!raw_route.alt_source_traversed_in_reverse.empty());
            super::alternate_description_factory.SetStartSegment(
                raw_route.segment_end_coordinates.front().source_phantom,
                raw_route.alt_source_traversed_in_reverse.front());
            // Get all the coordinates for the computed route
            for (const PathData &path_data : raw_route.unpacked_alternative)
            {
                super::current = super::facade->GetCoordinateOfNode(path_data.node);
                super::alternate_description_factory.AppendSegment(super::current, path_data);
            }
            super::alternate_description_factory.SetEndSegment(
                raw_route.segment_end_coordinates.back().target_phantom,
                raw_route.alt_source_traversed_in_reverse.back());
            super::alternate_description_factory.Run(super::config.zoom_level);

            if (super::config.geometry)
            {
                std::string alternateGeometry;
                alternateGeometry =
                    super::alternate_description_factory.AppendEncodedPolylineStringEncoded();
                alternative_route.set_route_geometry(alternateGeometry);
            }
            // Generate instructions for each alternative (simulated here)
            if (super::config.instructions)
            {
                BuildTextualDescription(raw_route.alternative_path_length,
                                        super::alternate_description_factory,
                                        super::alternative_path_segments,
                                        alternative_route);
            }

            super::alternate_description_factory.BuildRouteSummary(
                super::alternate_description_factory.get_entire_length(),
                raw_route.alternative_path_length);

            protobufResponse::RouteSummary alternative_routeSummary;

            alternative_routeSummary.set_total_distance(
                super::alternate_description_factory.summary.distance);
            alternative_routeSummary.set_total_time(
                super::alternate_description_factory.summary.duration);
            alternative_routeSummary.set_start_point(super::facade->GetEscapedNameForNameID(
                super::alternate_description_factory.summary.source_name_id));
            alternative_routeSummary.set_end_point(super::facade->GetEscapedNameForNameID(
                super::alternate_description_factory.summary.target_name_id));
            alternative_route.mutable_route_summary()->CopyFrom(route_summary);

            const std::vector<unsigned> &alternate_leg_end_indices =
                super::alternate_description_factory.GetViaIndices();
            for (const auto v : alternate_leg_end_indices)
            {
                alternative_route.add_via_indices(v);
            }

            alternative_route.add_route_name(route_names.alternative_path_name_1);
            alternative_route.add_route_name(route_names.alternative_path_name_2);

            response.mutable_alternative_route()->CopyFrom(main_route);
        }

        main_route.add_route_name(route_names.shortest_path_name_1);
        main_route.add_route_name(route_names.shortest_path_name_2);

        protobufResponse::Hint hint;
        hint.set_check_sum(super::facade->GetCheckSum());
        std::string res;
        for (const auto i : osrm::irange<std::size_t>(0, raw_route.segment_end_coordinates.size()))
        {
            ObjectEncoder::EncodeToBase64(raw_route.segment_end_coordinates[i].source_phantom, res);
            hint.add_location(res);
        }
        ObjectEncoder::EncodeToBase64(raw_route.segment_end_coordinates.back().target_phantom, res);
        hint.add_location(res);

        response.mutable_hint()->CopyFrom(hint);
        response.mutable_main_route()->CopyFrom(main_route);

        SimpleLogger().Write(logDEBUG) << response.DebugString();

        response.SerializeToString(&output);
        result_vector.insert(result_vector.end(), output.begin(), output.end());
    }
};
#endif // PROTOBUF_DESCRIPTOR_HPP
