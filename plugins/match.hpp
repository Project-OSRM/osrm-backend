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
        (void)removed_points; // unused

        const double distance_feature = -std::log(trace_length) + std::log(matched_length);

        // matched to the same point
        if (!std::isfinite(distance_feature))
        {
            return std::make_pair(ClassifierT::ClassLabel::NEGATIVE, 1.0);
        }

        const auto label_with_confidence = classifier.classify(distance_feature);

        return label_with_confidence;
    }

    bool getCandidates(const std::vector<FixedPointCoordinate> &input_coords,
                      const std::vector<std::pair<const int,const boost::optional<int>>> &input_bearings,
                      const double gps_precision,
                      std::vector<double> &sub_trace_lengths,
                      osrm::matching::CandidateLists &candidates_lists)
    {
        double query_radius = 10 * gps_precision;
        double last_distance = coordinate_calculation::haversine_distance(input_coords[0], input_coords[1]);

        sub_trace_lengths.resize(input_coords.size());
        sub_trace_lengths[0] = 0;
        for (const auto current_coordinate : osrm::irange<std::size_t>(0, input_coords.size()))
        {
            bool allow_uturn = false;
            if (0 < current_coordinate)
            {
                last_distance = coordinate_calculation::haversine_distance(input_coords[current_coordinate - 1], input_coords[current_coordinate]);

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
            // Use bearing values if supplied, otherwise fallback to 0,180 defaults
            auto bearing = input_bearings.size() > 0 ? input_bearings[current_coordinate].first : 0;
            auto range = input_bearings.size() > 0 ? (input_bearings[current_coordinate].second ? *input_bearings[current_coordinate].second : 10 ) : 180;
            facade->IncrementalFindPhantomNodeForCoordinateWithMaxDistance(
                    input_coords[current_coordinate], candidates, query_radius,
                    bearing, range);

            // sort by foward id, then by reverse id and then by distance
            std::sort(candidates.begin(), candidates.end(),
                [](const std::pair<PhantomNode, double>& lhs, const std::pair<PhantomNode, double>& rhs) {
                    return lhs.first.forward_node_id < rhs.first.forward_node_id ||
                            (lhs.first.forward_node_id == rhs.first.forward_node_id &&
                             (lhs.first.reverse_node_id < rhs.first.reverse_node_id ||
                              (lhs.first.reverse_node_id == rhs.first.reverse_node_id &&
                               lhs.second < rhs.second)));
                });

            auto new_end = std::unique(candidates.begin(), candidates.end(),
                [](const std::pair<PhantomNode, double>& lhs, const std::pair<PhantomNode, double>& rhs) {
                    return lhs.first.forward_node_id == rhs.first.forward_node_id &&
                           lhs.first.reverse_node_id == rhs.first.reverse_node_id;
                });
            candidates.resize(new_end - candidates.begin());

            if (!allow_uturn)
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
            }

            // sort by distance to make pruning effective
            std::sort(candidates.begin(), candidates.end(),
                [](const std::pair<PhantomNode, double>& lhs, const std::pair<PhantomNode, double>& rhs) {
                    return lhs.second < rhs.second;
                });

            candidates_lists.push_back(std::move(candidates));
        }

        return true;
    }

    osrm::json::Object submatchingToJSON(const osrm::matching::SubMatching &sub,
                                         const RouteParameters &route_parameters,
                                         const InternalRouteResult &raw_route)
    {
        osrm::json::Object subtrace;

        if (route_parameters.classify)
        {
            subtrace.values["confidence"] = sub.confidence;
        }

        JSONDescriptor<DataFacadeT> json_descriptor(facade);
        json_descriptor.SetConfig(route_parameters);

        subtrace.values["hint_data"] = json_descriptor.BuildHintData(raw_route);

        if (route_parameters.geometry || route_parameters.print_instructions)
        {
            DescriptionFactory factory;
            FixedPointCoordinate current_coordinate;
            factory.SetStartSegment(raw_route.segment_end_coordinates.front().source_phantom,
                                    raw_route.source_traversed_in_reverse.front());
            for (const auto i :
                 osrm::irange<std::size_t>(0, raw_route.unpacked_path_segments.size()))
            {
                for (const PathData &path_data : raw_route.unpacked_path_segments[i])
                {
                    current_coordinate = facade->GetCoordinateOfNode(path_data.node);
                    factory.AppendSegment(current_coordinate, path_data);
                }
                factory.SetEndSegment(raw_route.segment_end_coordinates[i].target_phantom,
                                      raw_route.target_traversed_in_reverse[i],
                                      raw_route.is_via_leg(i));
            }

            factory.Run(route_parameters.zoom_level);

            // we need because we don't run path simplification
            for (auto &segment : factory.path_description)
            {
                segment.necessary = true;
            }

            if (route_parameters.geometry)
            {
                subtrace.values["geometry"] = factory.AppendGeometryString(route_parameters.compression);
            }


            if (route_parameters.print_instructions)
            {
                std::vector<typename JSONDescriptor<DataFacadeT>::Segment> temp_segments;
                subtrace.values["instructions"] = json_descriptor.BuildTextualDescription(factory, temp_segments);
            }

            factory.BuildRouteSummary(factory.get_entire_length(),
                                              raw_route.shortest_path_length);
            osrm::json::Object json_route_summary;
            json_route_summary.values["total_distance"] = factory.summary.distance;
            json_route_summary.values["total_time"] = factory.summary.duration;
            subtrace.values["route_summary"] = json_route_summary;
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

        osrm::json::Array names;
        for (const auto &node : sub.nodes)
        {
            names.values.emplace_back( facade->get_name_for_id(node.name_id) );
        }
        subtrace.values["matched_names"] = names;

        return subtrace;
    }

    int HandleRequest(const RouteParameters &route_parameters,
                      osrm::json::Object &json_result) final override
    {
        // check number of parameters
        if (!check_all_coordinates(route_parameters.coordinates))
        {
            json_result.values["status"] = 400;
            json_result.values["status_message"] = "Invalid coordinates.";
            return 400;
        }

        std::vector<double> sub_trace_lengths;
        osrm::matching::CandidateLists candidates_lists;
        const auto &input_coords = route_parameters.coordinates;
        const auto &input_timestamps = route_parameters.timestamps;
        const auto &input_bearings = route_parameters.bearings;
        if (input_timestamps.size() > 0 && input_coords.size() != input_timestamps.size())
        {
            json_result.values["status"] = 400;
            json_result.values["status_message"] = "Number of timestamps does not match number of coordinates.";
            return 400;
        }

        if (input_bearings.size() > 0 && input_coords.size() != input_bearings.size())
        {
            json_result.values["status"] = 400;
            json_result.values["status_message"] = "Number of bearings does not match number of coordinates.";
            return 400;
        }

        // enforce maximum number of locations for performance reasons
        if (max_locations_map_matching > 0 &&
            static_cast<int>(input_coords.size()) < max_locations_map_matching)
        {
            json_result.values["status"] = 400;
            json_result.values["status_message"] = "Too many coodindates.";
            return 400;
        }

        // enforce maximum number of locations for performance reasons
        if (static_cast<int>(input_coords.size()) < 2)
        {
            json_result.values["status"] = 400;
            json_result.values["status_message"] = "At least two coordinates needed.";
            return 400;
        }

        const bool found_candidates =
            getCandidates(input_coords, input_bearings, route_parameters.gps_precision, sub_trace_lengths, candidates_lists);
        if (!found_candidates)
        {
            json_result.values["status"] = 400;
            json_result.values["status_message"] = "No suitable matching candidates found.";
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

        osrm::json::Array matchings;
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

            BOOST_ASSERT(raw_route.shortest_path_length != INVALID_EDGE_WEIGHT);

            matchings.values.emplace_back(submatchingToJSON(sub, route_parameters, raw_route));
        }

        if (osrm::json::Logger::get())
            osrm::json::Logger::get()->render("matching", json_result);
        json_result.values["matchings"] = matchings;

        if (sub_matchings.empty())
        {
            json_result.values["status"] = 0;
            json_result.values["status_message"] = "Found matchings.";
        }
        else
        {
            json_result.values["status"] = 207;
            json_result.values["status_message"] = "Cannot find matchings.";
        }

        return 200;
    }

  private:
    std::string descriptor_string;
    DataFacadeT *facade;
    int max_locations_map_matching;
    ClassifierT classifier;
};

#endif // MATCH_HPP
