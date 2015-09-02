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

#ifndef MATCH_HPP
#define MATCH_HPP

#include "plugin_base.hpp"

#include "../algorithms/bayes_classifier.hpp"
#include "../algorithms/object_encoder.hpp"
#include "../data_structures/search_engine.hpp"
#include "../descriptors/descriptor_base.hpp"
#include "../descriptors/json_descriptor.hpp"
#include "../routing_algorithms/map_matching.hpp"
#include "../util/compute_angle.hpp"
#include "../util/integer_range.hpp"
#include "../util/json_logger.hpp"
#include "../util/json_util.hpp"
#include "../util/string_util.hpp"

#include <cstdlib>

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

template <class DataFacadeT> class MapMatchingPlugin : public BasePlugin
{
    constexpr static const unsigned max_number_of_candidates = 10;

    std::shared_ptr<SearchEngine<DataFacadeT>> search_engine_ptr;

    using ClassifierT = BayesClassifier<LaplaceDistribution, LaplaceDistribution, double>;
    using TraceClassification = ClassifierT::ClassificationT;

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
    std::vector<Segment> shortest_path_segments;
    unsigned entered_restricted_area_count;
    unsigned necessary_segments_running_index;

  public:
    MapMatchingPlugin(DataFacadeT *facade, const int max_locations_map_matching)
        : descriptor_string("match"), facade(facade),
          max_locations_map_matching(max_locations_map_matching),
          // the values where derived from fitting a laplace distribution
          // to the values of manually classified traces
          classifier(LaplaceDistribution(0.005986, 0.016646),
                     LaplaceDistribution(0.054385, 0.458432),
                     0.696774) // valid apriori probability
    {
        search_engine_ptr = std::make_shared<SearchEngine<DataFacadeT>>(facade);
    }

    virtual ~MapMatchingPlugin() {}

    const std::string GetDescriptor() const final override { return descriptor_string; }

    TraceClassification
    classify(const float trace_length, const float matched_length, const int removed_points) const
    {
        const double distance_feature = -std::log(trace_length) + std::log(matched_length);

        // matched to the same point
        if (!std::isfinite(distance_feature))
        {
            return std::make_pair(ClassifierT::ClassLabel::NEGATIVE, 1.0);
        }

        const auto label_with_confidence = classifier.classify(distance_feature);

        return label_with_confidence;
    }

    bool getCandiates(const std::vector<FixedPointCoordinate> &input_coords,
                      std::vector<double> &sub_trace_lengths,
                      osrm::matching::CandidateLists &candidates_lists)
    {
        double last_distance =
            coordinate_calculation::great_circle_distance(input_coords[0], input_coords[1]);
        sub_trace_lengths.resize(input_coords.size());
        sub_trace_lengths[0] = 0;
        for (const auto current_coordinate : osrm::irange<std::size_t>(0, input_coords.size()))
        {
            bool allow_uturn = false;
            if (0 < current_coordinate)
            {
                last_distance = coordinate_calculation::great_circle_distance(
                    input_coords[current_coordinate - 1], input_coords[current_coordinate]);
                sub_trace_lengths[current_coordinate] +=
                    sub_trace_lengths[current_coordinate - 1] + last_distance;
            }

            if (input_coords.size() - 1 > current_coordinate && 0 < current_coordinate)
            {
                double turn_angle = ComputeAngle::OfThreeFixedPointCoordinates(
                    input_coords[current_coordinate - 1], input_coords[current_coordinate],
                    input_coords[current_coordinate + 1]);

                // sharp turns indicate a possible uturn
                if (turn_angle <= 90.0 || turn_angle >= 270.0)
                {
                    allow_uturn = true;
                }
            }

            std::vector<std::pair<PhantomNode, double>> candidates;
            if (!facade->IncrementalFindPhantomNodeForCoordinateWithMaxDistance(
                    input_coords[current_coordinate], candidates, last_distance / 2.0, 5,
                    max_number_of_candidates))
            {
                return false;
            }

            if (allow_uturn)
            {
                candidates_lists.push_back(candidates);
            }
            else
            {
                const auto compact_size = candidates.size();
                for (const auto i : osrm::irange<std::size_t>(0, compact_size))
                {
                    // Split edge if it is bidirectional and append reverse direction to end of list
                    if (candidates[i].first.forward_node_id != SPECIAL_NODEID &&
                        candidates[i].first.reverse_node_id != SPECIAL_NODEID)
                    {
                        PhantomNode reverse_node(candidates[i].first);
                        reverse_node.forward_node_id = SPECIAL_NODEID;
                        candidates.push_back(std::make_pair(reverse_node, candidates[i].second));

                        candidates[i].first.reverse_node_id = SPECIAL_NODEID;
                    }
                }
                candidates_lists.push_back(candidates);
            }
        }

        return true;
    }

    void getRoutingInstructions(SegmentInformation &segment,
                                struct RoundAbout &round_about,
                                std::string &temp_instruction,
                                osrm::json::Array &json_instruction_array)
    {
            osrm::json::Array json_instruction_row;
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
                        temp_instruction = cast::integral_to_string(
                            cast::enum_to_underlying(TurnInstruction::EnterRoundAbout));
                        current_turn_instruction += temp_instruction;
                        current_turn_instruction += "-";
                        temp_instruction = cast::integral_to_string(round_about.leave_at_exit + 1);
                        current_turn_instruction += temp_instruction;
                        round_about.leave_at_exit = 0;
                    }
                    else
                    {
                        temp_instruction =
                            cast::integral_to_string(cast::enum_to_underlying(current_instruction));
                        current_turn_instruction += temp_instruction;
                    }
                    json_instruction_row.values.push_back(current_turn_instruction);

                    json_instruction_row.values.push_back(facade->get_name_for_id(segment.name_id));
                    json_instruction_row.values.push_back(std::round(segment.length));
                    json_instruction_row.values.push_back(necessary_segments_running_index);
                    json_instruction_row.values.push_back(std::round(segment.duration / 10.));
                    json_instruction_row.values.push_back(
                        cast::integral_to_string(static_cast<unsigned>(segment.length)) + "m");
                    const double bearing_value = (segment.bearing / 10.);
                    json_instruction_row.values.push_back(bearing::get(bearing_value));
                    json_instruction_row.values.push_back(
                        static_cast<unsigned>(round(bearing_value)));
                    json_instruction_row.values.push_back(segment.travel_mode);

                    shortest_path_segments.emplace_back(
                        segment.name_id, static_cast<int>(segment.length),
                        static_cast<unsigned>(shortest_path_segments.size()));
                    json_instruction_array.values.push_back(json_instruction_row);
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
    
    void setDestinationPoint(osrm::json::Array &json_instruction_array)
    {
        std::string temp_instruction;
        osrm::json::Array json_last_instruction_row;
        temp_instruction = cast::integral_to_string(
            cast::enum_to_underlying(TurnInstruction::ReachedYourDestination));
        json_last_instruction_row.values.push_back(temp_instruction);
        json_last_instruction_row.values.push_back("");
        json_last_instruction_row.values.push_back(0);
        json_last_instruction_row.values.push_back(necessary_segments_running_index - 1);
        json_last_instruction_row.values.push_back(0);
        json_last_instruction_row.values.push_back("0m");
        json_last_instruction_row.values.push_back(bearing::get(0.0));
        json_last_instruction_row.values.push_back(0.);
        json_instruction_array.values.push_back(json_last_instruction_row);
    }
    
    void buildRouteSummary(osrm::json::Object &json_result,
                           DescriptionFactory::RouteSummary &route_summary)
    {
            osrm::json::Object json_route_summary;
            json_route_summary.values["total_distance"] = route_summary.distance;
            json_route_summary.values["total_time"] = route_summary.duration;
            json_route_summary.values["start_point"] =
                facade->get_name_for_id(route_summary.source_name_id);
            json_route_summary.values["end_point"] =
                facade->get_name_for_id(route_summary.target_name_id);
            json_result.values["route_summary"] = json_route_summary;
    }
    
    osrm::json::Object submatchingToJSON(const osrm::matching::SubMatching &sub,
                                         const RouteParameters &route_parameters,
                                         const InternalRouteResult &raw_route,
                                         osrm::json::Array &json_instruction_array,
                                         DescriptionFactory::RouteSummary &route_summary)
    {
        osrm::json::Object subtrace;

        if (route_parameters.classify)
        {
            subtrace.values["confidence"] = sub.confidence;
        }

        if (route_parameters.geometry)
        {
            // prevent multiple start points - set ViaPoints instead
            bool is_first_segment;
            if(route_parameters.print_instructions)
            {
                is_first_segment = (route_summary.distance != 0 ? false : true);
            }
            else
            {
                is_first_segment = true;
            }

            DescriptionFactory factory;
            FixedPointCoordinate current_coordinate;
            factory.SetStartSegment(raw_route.segment_end_coordinates.front().source_phantom,
                                    raw_route.source_traversed_in_reverse.front(), is_first_segment);
            for (const auto i :
                 osrm::irange<std::size_t>(0, raw_route.unpacked_path_segments.size()))
            {
                for (const PathData &path_data : raw_route.unpacked_path_segments[i])
                {
                    current_coordinate = facade->GetCoordinateOfNode(path_data.node);
                    factory.AppendSegment(current_coordinate, path_data);
                }
                // in some cases the map matching alogrithm creates lots of ViaPoints
                // these points aren't very user-friendly, so instead of checking the is_via_leg function
                // we always set it to false
                factory.SetEndSegment(raw_route.segment_end_coordinates[i].target_phantom,
                                      raw_route.target_traversed_in_reverse[i],
                                      false);
            }
            // we run this to get the distance for the instructions
            factory.Run(route_parameters.zoom_level);

            necessary_segments_running_index = 0;
            round_about.leave_at_exit = 0;
            round_about.name_id = 0;
            std::string temp_instruction;
            for (auto &segment : factory.path_description)
            {
                // we need this because we don't run DP
                segment.necessary = true;

                if(route_parameters.print_instructions)
                {
                    getRoutingInstructions(segment, round_about,
                                       temp_instruction,
                                       json_instruction_array);
                }
            }
            // build route summary
            factory.BuildRouteSummary(factory.get_entire_length(),
                raw_route.shortest_path_length);
            route_summary.distance += factory.summary.distance;
            route_summary.duration += factory.summary.duration;

            subtrace.values["geometry"] =
                factory.AppendGeometryString(route_parameters.compression);
        }

        subtrace.values["indices"] = osrm::json::make_array(sub.indices);

        osrm::json::Array points;
        for (const auto &node : sub.nodes)
        {
            points.values.emplace_back(
                osrm::json::make_array(node.location.lat / COORDINATE_PRECISION,
                                       node.location.lon / COORDINATE_PRECISION));
        }
        subtrace.values["matched_points"] = points;

        return subtrace;
    }

    int HandleRequest(const RouteParameters &route_parameters,
                      osrm::json::Object &json_result) final override
    {
        // check number of parameters
        if (!check_all_coordinates(route_parameters.coordinates))
        {
            return 400;
        }

        std::vector<double> sub_trace_lengths;
        osrm::matching::CandidateLists candidates_lists;
        const auto &input_coords = route_parameters.coordinates;
        const auto &input_timestamps = route_parameters.timestamps;
        if (input_timestamps.size() > 0 && input_coords.size() != input_timestamps.size())
        {
            return 400;
        }

        // enforce maximum number of locations for performance reasons
        if (max_locations_map_matching > 0 &&
            static_cast<int>(input_coords.size()) < max_locations_map_matching)
        {
            return 400;
        }

        const bool found_candidates =
            getCandiates(input_coords, sub_trace_lengths, candidates_lists);
        if (!found_candidates)
        {
            return 400;
        }

        // setup logging if enabled
        if (osrm::json::Logger::get())
            osrm::json::Logger::get()->initialize("matching");

        // call the actual map matching
        osrm::matching::SubMatchingList sub_matchings;
        search_engine_ptr->map_matching(candidates_lists, input_coords, input_timestamps,
                                        route_parameters.matching_beta,
                                        route_parameters.gps_precision, sub_matchings);

        if (sub_matchings.empty())
        {
            return 400;
        }

        osrm::json::Array matchings;
        osrm::json::Array json_instruction_array;
        DescriptionFactory::RouteSummary route_summary;
        if(route_parameters.print_instructions)
        {
            route_summary.distance = 0;
            route_summary.duration = 0;
            route_summary.source_name_id = sub_matchings.front().nodes.front().name_id;
	    route_summary.target_name_id = sub_matchings.back().nodes.back().name_id;
        }
        for (auto &sub : sub_matchings)
        {
            // classify result
            if (route_parameters.classify)
            {
                double trace_length =
                    sub_trace_lengths[sub.indices.back()] - sub_trace_lengths[sub.indices.front()];
                TraceClassification classification =
                    classify(trace_length, sub.length,
                             (sub.indices.back() - sub.indices.front() + 1) - sub.nodes.size());
                if (classification.first == ClassifierT::ClassLabel::POSITIVE)
                {
                    sub.confidence = classification.second;
                }
                else
                {
                    sub.confidence = 1 - classification.second;
                }
            }

            BOOST_ASSERT(sub.nodes.size() > 1);

            // FIXME we only run this to obtain the geometry
            // The clean way would be to get this directly from the map matching plugin
            InternalRouteResult raw_route;
            PhantomNodes current_phantom_node_pair;
            for (unsigned i = 0; i < sub.nodes.size() - 1; ++i)
            {
                current_phantom_node_pair.source_phantom = sub.nodes[i];
                current_phantom_node_pair.target_phantom = sub.nodes[i + 1];
                raw_route.segment_end_coordinates.emplace_back(current_phantom_node_pair);
            }
            search_engine_ptr->shortest_path(
                raw_route.segment_end_coordinates,
                std::vector<bool>(raw_route.segment_end_coordinates.size(), true), raw_route);

            matchings.values.emplace_back(submatchingToJSON(sub, route_parameters, raw_route,
                json_instruction_array, route_summary));
        }
        if(route_parameters.print_instructions)
        {
            setDestinationPoint(json_instruction_array);
            json_result.values["route_instructions"] = json_instruction_array;
            buildRouteSummary(json_result, route_summary);
        }

        if (osrm::json::Logger::get())
            osrm::json::Logger::get()->render("matching", json_result);
        json_result.values["matchings"] = matchings;

        return 200;
    }

  private:
    std::string descriptor_string;
    DataFacadeT *facade;
    int max_locations_map_matching;
    ClassifierT classifier;
};

#endif // MATCH_HPP
