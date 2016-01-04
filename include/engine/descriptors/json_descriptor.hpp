#ifndef JSON_DESCRIPTOR_HPP
#define JSON_DESCRIPTOR_HPP

#include "engine/descriptors/descriptor_base.hpp"
#include "engine/descriptors/description_factory.hpp"
#include "engine/object_encoder.hpp"
#include "engine/route_name_extraction.hpp"
#include "engine/segment_information.hpp"
#include "extractor/turn_instructions.hpp"
#include "util/bearing.hpp"
#include "util/cast.hpp"
#include "util/integer_range.hpp"
#include "util/json_renderer.hpp"
#include "util/simple_logger.hpp"
#include "util/string_util.hpp"
#include "util/timing_util.hpp"

#include "osrm/json_container.hpp"

#include <limits>
#include <algorithm>
#include <string>

template <class DataFacadeT> class JSONDescriptor final : public BaseDescriptor<DataFacadeT>
{
  private:
    DataFacadeT *facade;
    DescriptorConfig config;
    DescriptionFactory description_factory, alternate_description_factory;
    FixedPointCoordinate current;

  public:
    struct Segment
    {
        Segment() : name_id(INVALID_NAMEID), length(-1), position(0) {}
        Segment(unsigned n, int l, unsigned p) : name_id(n), length(l), position(p) {}
        unsigned name_id;
        int length;
        unsigned position;
    };
  private:
    std::vector<Segment> shortest_path_segments, alternative_path_segments;
    ExtractRouteNames<DataFacadeT, Segment> GenerateRouteNames;

  public:
    explicit JSONDescriptor(DataFacadeT *facade) : facade(facade)
    {
    }

    virtual void SetConfig(const DescriptorConfig &c) override final { config = c; }

    unsigned DescribeLeg(const std::vector<PathData> &route_leg,
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
        description_factory.SetEndSegment(leg_phantoms.target_phantom, target_traversed_in_reverse,
                                          is_via_leg);
        ++added_element_count;
        BOOST_ASSERT((route_leg.size() + 1) == added_element_count);
        return added_element_count;
    }

    virtual void Run(const InternalRouteResult &raw_route,
                     osrm::json::Object &json_result) override final
    {
        if (INVALID_EDGE_WEIGHT == raw_route.shortest_path_length)
        {
            // We do not need to do much, if there is no route ;-)
            return;
        }

        // check if first segment is non-zero
        BOOST_ASSERT(raw_route.unpacked_path_segments.size() ==
                     raw_route.segment_end_coordinates.size());

        description_factory.SetStartSegment(
            raw_route.segment_end_coordinates.front().source_phantom,
            raw_route.source_traversed_in_reverse.front());

        // for each unpacked segment add the leg to the description
        for (const auto i : osrm::irange<std::size_t>(0, raw_route.unpacked_path_segments.size()))
        {
#ifndef NDEBUG
            const int added_segments =
#endif
                DescribeLeg(raw_route.unpacked_path_segments[i],
                            raw_route.segment_end_coordinates[i],
                            raw_route.target_traversed_in_reverse[i], raw_route.is_via_leg(i));
            BOOST_ASSERT(0 < added_segments);
        }
        description_factory.Run(config.zoom_level);

        if (config.geometry)
        {
            osrm::json::Value route_geometry =
                description_factory.AppendGeometryString(config.encode_geometry);
            json_result.values["route_geometry"] = route_geometry;
        }
        if (config.instructions)
        {
            osrm::json::Array json_route_instructions = BuildTextualDescription(description_factory, shortest_path_segments);
            json_result.values["route_instructions"] = json_route_instructions;
        }
        description_factory.BuildRouteSummary(description_factory.get_entire_length(),
                                              raw_route.shortest_path_length);
        osrm::json::Object json_route_summary;
        json_route_summary.values["total_distance"] = description_factory.summary.distance;
        json_route_summary.values["total_time"] = description_factory.summary.duration;
        json_route_summary.values["start_point"] =
            facade->get_name_for_id(description_factory.summary.source_name_id);
        json_route_summary.values["end_point"] =
            facade->get_name_for_id(description_factory.summary.target_name_id);
        json_result.values["route_summary"] = json_route_summary;

        BOOST_ASSERT(!raw_route.segment_end_coordinates.empty());

        osrm::json::Array json_via_points_array;
        osrm::json::Array json_first_coordinate;
        json_first_coordinate.values.push_back(
            raw_route.segment_end_coordinates.front().source_phantom.location.lat /
            COORDINATE_PRECISION);
        json_first_coordinate.values.push_back(
            raw_route.segment_end_coordinates.front().source_phantom.location.lon /
            COORDINATE_PRECISION);
        json_via_points_array.values.push_back(json_first_coordinate);
        for (const PhantomNodes &nodes : raw_route.segment_end_coordinates)
        {
            std::string tmp;
            osrm::json::Array json_coordinate;
            json_coordinate.values.push_back(nodes.target_phantom.location.lat /
                                             COORDINATE_PRECISION);
            json_coordinate.values.push_back(nodes.target_phantom.location.lon /
                                             COORDINATE_PRECISION);
            json_via_points_array.values.push_back(json_coordinate);
        }
        json_result.values["via_points"] = json_via_points_array;

        osrm::json::Array json_via_indices_array;

        std::vector<unsigned> const &shortest_leg_end_indices = description_factory.GetViaIndices();
        json_via_indices_array.values.insert(json_via_indices_array.values.end(),
                                             shortest_leg_end_indices.begin(),
                                             shortest_leg_end_indices.end());
        json_result.values["via_indices"] = json_via_indices_array;

        // only one alternative route is computed at this time, so this is hardcoded
        if (INVALID_EDGE_WEIGHT != raw_route.alternative_path_length)
        {
            json_result.values["found_alternative"] = osrm::json::True();
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
            alternate_description_factory.SetEndSegment(
                raw_route.segment_end_coordinates.back().target_phantom,
                raw_route.alt_source_traversed_in_reverse.back());
            alternate_description_factory.Run(config.zoom_level);

            if (config.geometry)
            {
                osrm::json::Value alternate_geometry_string =
                    alternate_description_factory.AppendGeometryString(config.encode_geometry);
                osrm::json::Array json_alternate_geometries_array;
                json_alternate_geometries_array.values.push_back(alternate_geometry_string);
                json_result.values["alternative_geometries"] = json_alternate_geometries_array;
            }
            // Generate instructions for each alternative (simulated here)
            osrm::json::Array json_alt_instructions;
            osrm::json::Array json_current_alt_instructions;
            if (config.instructions)
            {
                json_current_alt_instructions = BuildTextualDescription(alternate_description_factory, alternative_path_segments);
                json_alt_instructions.values.push_back(json_current_alt_instructions);
                json_result.values["alternative_instructions"] = json_alt_instructions;
            }
            alternate_description_factory.BuildRouteSummary(
                alternate_description_factory.get_entire_length(),
                raw_route.alternative_path_length);

            osrm::json::Object json_alternate_route_summary;
            osrm::json::Array json_alternate_route_summary_array;
            json_alternate_route_summary.values["total_distance"] =
                alternate_description_factory.summary.distance;
            json_alternate_route_summary.values["total_time"] =
                alternate_description_factory.summary.duration;
            json_alternate_route_summary.values["start_point"] =
                facade->get_name_for_id(alternate_description_factory.summary.source_name_id);
            json_alternate_route_summary.values["end_point"] =
                facade->get_name_for_id(alternate_description_factory.summary.target_name_id);
            json_alternate_route_summary_array.values.push_back(json_alternate_route_summary);
            json_result.values["alternative_summaries"] = json_alternate_route_summary_array;

            std::vector<unsigned> const &alternate_leg_end_indices =
                alternate_description_factory.GetViaIndices();
            osrm::json::Array json_altenative_indices_array;
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
            GenerateRouteNames(shortest_path_segments, alternative_path_segments, facade);
        osrm::json::Array json_route_names;
        json_route_names.values.push_back(route_names.shortest_path_name_1);
        json_route_names.values.push_back(route_names.shortest_path_name_2);
        json_result.values["route_name"] = json_route_names;

        if (INVALID_EDGE_WEIGHT != raw_route.alternative_path_length)
        {
            osrm::json::Array json_alternate_names_array;
            osrm::json::Array json_alternate_names;
            json_alternate_names.values.push_back(route_names.alternative_path_name_1);
            json_alternate_names.values.push_back(route_names.alternative_path_name_2);
            json_alternate_names_array.values.push_back(json_alternate_names);
            json_result.values["alternative_names"] = json_alternate_names_array;
        }

        json_result.values["hint_data"] = BuildHintData(raw_route);
    }

    inline osrm::json::Object BuildHintData(const InternalRouteResult& raw_route) const
    {
        osrm::json::Object json_hint_object;
        json_hint_object.values["checksum"] = facade->GetCheckSum();
        osrm::json::Array json_location_hint_array;
        std::string hint;
        for (const auto i : osrm::irange<std::size_t>(0, raw_route.segment_end_coordinates.size()))
        {
            ObjectEncoder::EncodeToBase64(raw_route.segment_end_coordinates[i].source_phantom,
                                          hint);
            json_location_hint_array.values.push_back(hint);
        }
        ObjectEncoder::EncodeToBase64(raw_route.segment_end_coordinates.back().target_phantom,
                                      hint);
        json_location_hint_array.values.push_back(hint);
        json_hint_object.values["locations"] = json_location_hint_array;

        return json_hint_object;
    }

    inline osrm::json::Array BuildTextualDescription(const DescriptionFactory &description_factory,
                                                     std::vector<Segment> &route_segments_list) const
    {
        osrm::json::Array json_instruction_array;
        // Segment information has following format:
        //["instruction id","streetname",length,position,time,"length","earth_direction",azimuth]
        unsigned necessary_segments_running_index = 0;

        struct RoundAbout
        {
            RoundAbout() : start_index(std::numeric_limits<int>::max()), name_id(INVALID_NAMEID), leave_at_exit(std::numeric_limits<int>::max()) {}
            int start_index;
            unsigned name_id;
            int leave_at_exit;
        } round_about;

        round_about.leave_at_exit = 0;
        round_about.name_id = 0;
        std::string temp_dist, temp_length, temp_duration, temp_bearing, temp_instruction;

        // Fetch data from Factory and generate a string from it.
        for (const SegmentInformation &segment : description_factory.path_description)
        {
            osrm::json::Array json_instruction_row;
            TurnInstruction current_instruction = segment.turn_instruction;
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
                        temp_instruction = std::to_string(
                            cast::enum_to_underlying(TurnInstruction::EnterRoundAbout));
                        current_turn_instruction += temp_instruction;
                        current_turn_instruction += "-";
                        temp_instruction = std::to_string(round_about.leave_at_exit + 1);
                        current_turn_instruction += temp_instruction;
                        round_about.leave_at_exit = 0;
                    }
                    else
                    {
                        temp_instruction =
                            std::to_string(cast::enum_to_underlying(current_instruction));
                        current_turn_instruction += temp_instruction;
                    }
                    json_instruction_row.values.push_back(current_turn_instruction);

                    json_instruction_row.values.push_back(facade->get_name_for_id(segment.name_id));
                    json_instruction_row.values.push_back(std::round(segment.length));
                    json_instruction_row.values.push_back(necessary_segments_running_index);
                    json_instruction_row.values.push_back(std::round(segment.duration / 10.));
                    json_instruction_row.values.push_back(
                        std::to_string(static_cast<unsigned>(segment.length)) + "m");

                    // post turn bearing
                    const double post_turn_bearing_value = (segment.post_turn_bearing / 10.);
                    json_instruction_row.values.push_back(bearing::get(post_turn_bearing_value));
                    json_instruction_row.values.push_back(
                        static_cast<unsigned>(round(post_turn_bearing_value)));

                    json_instruction_row.values.push_back(segment.travel_mode);

                    // pre turn bearing
                    const double pre_turn_bearing_value = (segment.pre_turn_bearing / 10.);
                    json_instruction_row.values.push_back(bearing::get(pre_turn_bearing_value));
                    json_instruction_row.values.push_back(
                        static_cast<unsigned>(round(pre_turn_bearing_value)));

                    json_instruction_array.values.push_back(json_instruction_row);

                    route_segments_list.emplace_back(
                        segment.name_id, static_cast<int>(segment.length),
                        static_cast<unsigned>(route_segments_list.size()));
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

        osrm::json::Array json_last_instruction_row;
        temp_instruction =
            std::to_string(cast::enum_to_underlying(TurnInstruction::ReachedYourDestination));
        json_last_instruction_row.values.push_back(temp_instruction);
        json_last_instruction_row.values.push_back("");
        json_last_instruction_row.values.push_back(0);
        json_last_instruction_row.values.push_back(necessary_segments_running_index - 1);
        json_last_instruction_row.values.push_back(0);
        json_last_instruction_row.values.push_back("0m");
        json_last_instruction_row.values.push_back(bearing::get(0.0));
        json_last_instruction_row.values.push_back(0.);
        json_last_instruction_row.values.push_back(bearing::get(0.0));
        json_last_instruction_row.values.push_back(0.);
        json_instruction_array.values.push_back(json_last_instruction_row);

        return json_instruction_array;
    }
};

#endif /* JSON_DESCRIPTOR_H_ */
