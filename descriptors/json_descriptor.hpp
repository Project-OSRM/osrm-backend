/*

Copyright (c) 2015, Project OSRM contributors
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

#ifndef JSON_DESCRIPTOR_HPP
#define JSON_DESCRIPTOR_HPP

#include "descriptor_base.hpp"
#include "description_factory.hpp"
#include "../algorithms/object_encoder.hpp"
#include "../algorithms/route_name_extraction.hpp"
#include "../data_structures/segment_information.hpp"
#include "../data_structures/turn_instructions.hpp"
#include "../util/bearing.hpp"
#include "../util/integer_range.hpp"
#include "../util/json_renderer.hpp"
#include "../util/simple_logger.hpp"
#include "../util/string_util.hpp"
#include "../util/timing_util.hpp"

#include <osrm/json_container.hpp>

#include <algorithm>

template <class DataFacadeT> class JSONDescriptor final : public BaseDescriptor<DataFacadeT>
{
    typedef BaseDescriptor<DataFacadeT> super;
    DescriptorConfig config;
    DescriptionFactory description_factory, alternate_description_factory;
    FixedPointCoordinate current;
    unsigned entered_restricted_area_count;
  public:

    explicit JSONDescriptor(DataFacadeT *facade) : super(facade), entered_restricted_area_count(0) {}

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
            current_coordinate = super::facade->GetCoordinateOfNode(path_data.node);
            description_factory.AppendSegment(current_coordinate, path_data);
            ++added_element_count;
        }
        description_factory.SetEndSegment(leg_phantoms.target_phantom, target_traversed_in_reverse,
                                          is_via_leg);
        ++added_element_count;
        BOOST_ASSERT((route_leg.size() + 1) == added_element_count);
        return added_element_count;
    }

    void Run(const RawRouteData &raw_route, http::Reply &reply) final
    {
        JSON::Object json_result;
        if (INVALID_EDGE_WEIGHT == raw_route.shortest_path_length)
        {
            // We do not need to do much, if there is no route ;-)
            json_result.values["status"] = 207;
            json_result.values["status_message"] = "Cannot find route between points";
            JSON::render(reply.content, json_result);
            return;
        }

        // check if first segment is non-zero
        std::string road_name = super::facade->GetEscapedNameForNameID(
            raw_route.segment_end_coordinates.front().source_phantom.name_id);

        BOOST_ASSERT(raw_route.unpacked_path_segments.size() ==
                     raw_route.segment_end_coordinates.size());

        super::description_factory.SetStartSegment(
            raw_route.segment_end_coordinates.front().source_phantom,
            raw_route.source_traversed_in_reverse.front());
        json_result.values["status"] = 0;
        json_result.values["status_message"] = "Found route between points";

        // for each unpacked segment add the leg to the description
        for (const auto i : osrm::irange<std::size_t>(0, raw_route.unpacked_path_segments.size()))
        {
#ifndef NDEBUG
            const int added_segments =
#endif
            super::DescribeLeg(
                        raw_route.unpacked_path_segments[i],
                        raw_route.segment_end_coordinates[i],
                        raw_route.target_traversed_in_reverse[i],
                        raw_route.is_via_leg(i));
            BOOST_ASSERT(0 < added_segments);
        }
        super::description_factory.Run(super::facade, super::config.zoom_level);

        if (super::config.geometry)
        {
            JSON::Value route_geometry =
            super::description_factory.AppendEncodedPolylineStringEncoded();
            json_result.values["route_geometry"] = route_geometry;
        }
        if (super::config.instructions)
        {
            JSON::Array json_route_instructions;
            BuildTextualDescription(super::description_factory,
                                    json_route_instructions,
                                    raw_route.shortest_path_length,
                                    super::shortest_path_segments);
            json_result.values["route_instructions"] = json_route_instructions;
        }
        super::description_factory.BuildRouteSummary(
                                              super::description_factory.get_entire_length(),
                                              raw_route.shortest_path_length);
        JSON::Object json_route_summary;
        json_route_summary.values["total_distance"] = super::description_factory.summary.distance;
        json_route_summary.values["total_time"] = super::description_factory.summary.duration;
        json_route_summary.values["start_point"] =
            super::facade->GetEscapedNameForNameID(super::description_factory.summary.source_name_id);
        json_route_summary.values["end_point"] =
            super::facade->GetEscapedNameForNameID(super::description_factory.summary.target_name_id);
        json_result.values["route_summary"] = json_route_summary;

        BOOST_ASSERT(!raw_route.segment_end_coordinates.empty());

        JSON::Array json_via_points_array;
        JSON::Array json_first_coordinate;
        json_first_coordinate.values.emplace_back(
            raw_route.segment_end_coordinates.front().source_phantom.location.lat /
            COORDINATE_PRECISION);
        json_first_coordinate.values.emplace_back(
            raw_route.segment_end_coordinates.front().source_phantom.location.lon /
            COORDINATE_PRECISION);
        json_via_points_array.values.emplace_back(json_first_coordinate);
        for (const PhantomNodes &nodes : raw_route.segment_end_coordinates)
        {
            std::string tmp;
            JSON::Array json_coordinate;
            json_coordinate.values.emplace_back(nodes.target_phantom.location.lat /
                                             COORDINATE_PRECISION);
            json_coordinate.values.emplace_back(nodes.target_phantom.location.lon /
                                             COORDINATE_PRECISION);
            json_via_points_array.values.emplace_back(json_coordinate);
        }
        json_result.values["via_points"] = json_via_points_array;

        osrm::json::Array json_via_indices_array;

        std::vector<unsigned> const &shortest_leg_end_indices = super::description_factory.GetViaIndices();
        json_via_indices_array.values.insert(json_via_indices_array.values.end(),
                                             shortest_leg_end_indices.begin(),
                                             shortest_leg_end_indices.end());
        json_result.values["via_indices"] = json_via_indices_array;

        // only one alternative route is computed at this time, so this is hardcoded
        if (INVALID_EDGE_WEIGHT != raw_route.alternative_path_length)
        {
            json_result.values["found_alternative"] = osrm::json::True();
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
            super::alternate_description_factory.Run(
                                                   super::facade, super::config.zoom_level);

            if (super::config.geometry)
            {
                JSON::Value alternate_geometry_string =
                super::alternate_description_factory.AppendEncodedPolylineStringEncoded();
                JSON::Array json_alternate_geometries_array;
                json_alternate_geometries_array.values.emplace_back(alternate_geometry_string);
                json_result.values["alternative_geometries"] = json_alternate_geometries_array;
            }
            // Generate instructions for each alternative (simulated here)
            JSON::Array json_alt_instructions;
            JSON::Array json_current_alt_instructions;
            if (super::config.instructions)
            {
                BuildTextualDescription(super::alternate_description_factory,
                                        json_current_alt_instructions,
                                        raw_route.alternative_path_length,
                                        super::alternative_path_segments);
                json_alt_instructions.values.emplace_back(json_current_alt_instructions);
                json_result.values["alternative_instructions"] = json_alt_instructions;
            }
            super::alternate_description_factory.BuildRouteSummary(
                super::alternate_description_factory.get_entire_length(), raw_route.alternative_path_length);

            osrm::json::Object json_alternate_route_summary;
            osrm::json::Array json_alternate_route_summary_array;
            json_alternate_route_summary.values["total_distance"] =
                super::alternate_description_factory.summary.distance;
            json_alternate_route_summary.values["total_time"] =
                super::alternate_description_factory.summary.duration;
            json_alternate_route_summary.values["start_point"] = super::facade->GetEscapedNameForNameID(
                super::alternate_description_factory.summary.source_name_id);
            json_alternate_route_summary.values["end_point"] = super::facade->GetEscapedNameForNameID(
                super::alternate_description_factory.summary.target_name_id);
            json_alternate_route_summary_array.values.emplace_back(json_alternate_route_summary);
            json_result.values["alternative_summaries"] = json_alternate_route_summary_array;

            std::vector<unsigned> const &alternate_leg_end_indices =
                super::alternate_description_factory.GetViaIndices();
            JSON::Array json_altenative_indices_array;
            json_altenative_indices_array.values.insert(json_altenative_indices_array.values.end(),
                                                        alternate_leg_end_indices.begin(),
                                                        alternate_leg_end_indices.end());
            json_result.values["alternative_indices"] = json_altenative_indices_array;
        }
        else
        {
            json_result.values["found_alternative"] = osrm::json::False();
        }

        // Get Names for both routes
        RouteNames route_names =
            super::GenerateRouteNames(super::shortest_path_segments, super::alternative_path_segments,
                super::facade);
        JSON::Array json_route_names;
        json_route_names.values.emplace_back(route_names.shortest_path_name_1);
        json_route_names.values.emplace_back(route_names.shortest_path_name_2);
        json_result.values["route_name"] = json_route_names;

        if (INVALID_EDGE_WEIGHT != raw_route.alternative_path_length)
        {
            JSON::Array json_alternate_names_array;
            JSON::Array json_alternate_names;
            json_alternate_names.values.emplace_back(route_names.alternative_path_name_1);
            json_alternate_names.values.emplace_back(route_names.alternative_path_name_2);
            json_alternate_names_array.values.emplace_back(json_alternate_names);
            json_result.values["alternative_names"] = json_alternate_names_array;
        }

        JSON::Object json_hint_object;
        json_hint_object.values["checksum"] = super::facade->GetCheckSum();
        JSON::Array json_location_hint_array;
        std::string hint;
        for (const auto i : osrm::irange<std::size_t>(0, raw_route.segment_end_coordinates.size()))
        {
            ObjectEncoder::EncodeToBase64(raw_route.segment_end_coordinates[i].source_phantom, hint);
            json_location_hint_array.values.emplace_back(hint);
        }
        ObjectEncoder::EncodeToBase64(raw_route.segment_end_coordinates.back().target_phantom, hint);
        json_location_hint_array.values.emplace_back(hint);
        json_hint_object.values["locations"] = json_location_hint_array;
        json_result.values["hint_data"] = json_hint_object;

        // render the content to the output array
        TIMER_START(route_render);
        JSON::render(reply.content, json_result);
        TIMER_STOP(route_render);
        SimpleLogger().Write(logDEBUG) << "rendering took: " << TIMER_MSEC(route_render);
    }

    // TODO: reorder parameters
    inline void BuildTextualDescription(DescriptionFactory &description_factory,
                                        osrm::json::Array &json_instruction_array,
                                        const int route_length,
                                        std::vector<typename super::Segment> &route_segments_list)
    {
        std::vector<typename super::Instruction> instructions;
        super::BuildTextualDescription(description_factory,
                                                         instructions,
                                                         route_length,
                                                         route_segments_list);

        for (const typename super::Instruction &i : instructions)
        {
            JSON::Array json_instruction_row;
            json_instruction_row.values.emplace_back(i.instruction_id);
            json_instruction_row.values.emplace_back(i.street_name);
            json_instruction_row.values.emplace_back(i.length);
            json_instruction_row.values.emplace_back(i.position);
            json_instruction_row.values.emplace_back(i.time);
            json_instruction_row.values.emplace_back(i.length_string);
            json_instruction_row.values.emplace_back(i.bearing);
            json_instruction_row.values.emplace_back(i.azimuth);
            json_instruction_row.values.emplace_back(i.travel_mode);
            json_instruction_array.values.emplace_back(json_instruction_row);
        }
    }
};

#endif /* JSON_DESCRIPTOR_HPP */
