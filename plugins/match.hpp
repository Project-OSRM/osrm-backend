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

    struct SavedCoordinates {
      FixedPointCoordinate coordinate;
      PathData data;
      bool new_angle;   // calculate a new angle for this coordinate
    };
    const int DEFAULT_MAX_DISTANCE = 200;
    const int DEFAULT_MIN_DELETE_DISTANCE = 30;

    // we need these two to check if the iterator is on the first or last element
    template <typename Iter, typename Cont>
    bool is_last(Iter iter, const Cont& cont)
    {
        return (iter != cont.end()) && (next(iter) == cont.end());
    }
    template <typename Iter, typename Cont>
    bool is_first(Iter iter, const Cont& cont)
    {
        return (iter != cont.begin()) && (prev(iter) == cont.begin());
    }

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

    const std::string GetDescriptor() const final { return descriptor_string; }

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

    double getNewAngle(typename std::list<SavedCoordinates>::iterator iterator,
                       std::list<SavedCoordinates> coordinate_list)
    {
        typename std::list<SavedCoordinates>::iterator prev_coordinate = iterator;
        typename std::list<SavedCoordinates>::iterator next_coordinate = iterator;
        advance(prev_coordinate, -1);
        advance(next_coordinate, 1);

        double angle = ComputeAngle::OfThreeFixedPointCoordinates(prev_coordinate->coordinate, iterator->coordinate, next_coordinate->coordinate);
        return angle;
    }
    
    bool checkDeleteRoute(typename std::list<SavedCoordinates>::iterator start_iter, 
                          typename std::list<SavedCoordinates>::iterator act_iter, 
                          std::list<SavedCoordinates> coordinate_list) 
    {
        if(is_last(start_iter, coordinate_list) && is_first(act_iter, coordinate_list) &&
           is_last(act_iter, coordinate_list) && is_first(start_iter, coordinate_list))
        {
            return false;
        }

        // special case: on every roundabout the map matching algorithm goes 2 times through it
        // one time it goes completly around and the other one it goes through the matched route
        if((start_iter->data.turn_instruction == TurnInstruction::EnterRoundAbout) || 
           (act_iter->data.turn_instruction == TurnInstruction::EnterRoundAbout))
        {
            return true;
        }

        // How to delete a path:
        // (the path goes through the coordinates x1, o, x2, x3, x2, o, x4)
        //   |
        //   x4   x3
        //   |    |
        //   o----x2 
        //   |
        //   x1
        // o is the checked coordinate
        // prev_point is described as: the previous coordinate of the iterator who is ahead (in the example: x2)
        // next_point is described as: the next coordinate of the iterator who doesn't move (in the example: x2)
        typename std::list<SavedCoordinates>::iterator prev_point = --act_iter;
        typename std::list<SavedCoordinates>::iterator next_point = ++start_iter;
        if(prev_point->coordinate.lon == next_point->coordinate.lon && prev_point->coordinate.lat == next_point->coordinate.lat)
        {
            // if both coordinates are equal change the positions and check again, but this 
            // time if these coordinates are NOT the same
            // example: prev_point is now the previous coordinate of the iterator who doesn't move (=> x1)
            // next_point is the next coordinate of the iterator who is ahead (=> x4)
            // this tells us that there is no new way which is created, but rather a unneccessary path
            prev_point = start_iter;
            next_point = act_iter;
            advance(prev_point, -2);
            advance(next_point, 2);

            if(prev_point->coordinate.lon != next_point->coordinate.lon && prev_point->coordinate.lat != next_point->coordinate.lat)
            {
                return true;
            }
        }

        return false;
    }
    
    void createAndCutGeometry(const osrm::matching::SubMatching &sub,
                              const RouteParameters &route_parameters,
                              const InternalRouteResult &raw_route,
                              DescriptionFactory &factory,
                              std::list<SavedCoordinates> &coordinate_list)
    {
        FixedPointCoordinate current_coordinate (0, 0);
        factory.SetStartSegment(raw_route.segment_end_coordinates.front().source_phantom,
                                raw_route.source_traversed_in_reverse.front());
        for (const auto i :
                osrm::irange<std::size_t>(0, raw_route.unpacked_path_segments.size()))
        {
            // add all coordinates from the segment
            for (const PathData &path_data : raw_route.unpacked_path_segments[i])
            {
                FixedPointCoordinate previous_coordinate = current_coordinate;
                current_coordinate = facade->GetCoordinateOfNode(path_data.node);
                SavedCoordinates coord = {current_coordinate, path_data, false};

                // we don't need multiple same coordinates
                if(previous_coordinate.lat != current_coordinate.lat && 
                    previous_coordinate.lon != current_coordinate.lon)
                {
                    coordinate_list.push_back(coord);
                }
                else
                {
                    // since we ignore the last coordinate we need to calculate a new angle
                    coordinate_list.back().new_angle = true;
                }
            }
        }

        // delete unneccessary paths
        bool delete_flag;
        for(typename std::list<SavedCoordinates>::iterator iter = coordinate_list.begin(); iter != coordinate_list.end(); )
        {
            delete_flag = false;
            double act_distance, act_max_distance = 0;
            typename std::list<SavedCoordinates>::iterator ii = iter;
            // this iterator goes ahead and checks all the other coordinates to find possible candidates
            // to remove (remove condition: both iterators have the same longitude and latitude value)
            for(advance(ii, 1); ii != coordinate_list.end(); )
            {
                // get the greatest distance from the actual loop
                act_distance = coordinate_calculation::great_circle_distance(iter->coordinate, ii->coordinate);
                if(act_distance > act_max_distance)
                {
                    act_max_distance = act_distance;
                }

                if(iter->coordinate.lon == ii->coordinate.lon && iter->coordinate.lat == ii->coordinate.lat)
                {
                    if((checkDeleteRoute(iter, ii, coordinate_list) && (act_max_distance <= DEFAULT_MAX_DISTANCE)) || 
                       (act_max_distance <= DEFAULT_MIN_DELETE_DISTANCE))
                    {
                        iter = coordinate_list.erase(iter, ii);
                        iter->new_angle = true;
                        delete_flag = true;
                        break;
                    }
                    else
                    {
                        break;
                    }
                }
                else
                {
                    ++ii;
                }
            }
            // if we deleted a path we have to check the same coordinate again,
            // because there may be other paths to delete
            if(!delete_flag)
            {
                ++iter;
            }
        }

        // Append
        for(typename std::list<SavedCoordinates>::iterator iter = coordinate_list.begin(); iter != coordinate_list.end(); ++iter)
        {
            current_coordinate = iter->coordinate;
            PathData new_data = iter->data;

            if(iter->new_angle)
            {
                // special case: current coordinate is first or last coordinate => we can't calculate a new angle
                if(!is_last(iter, coordinate_list) && !is_first(iter, coordinate_list) &&
                    iter != coordinate_list.begin() && iter != coordinate_list.end())
                {
                    double angle = getNewAngle(iter, coordinate_list);
                    new_data.turn_instruction = TurnInstructionsClass::GetTurnDirectionOfInstruction(angle);
                }
                else
                {
                    new_data.turn_instruction = TurnInstruction::ReachViaLocation;
                }
            }
            factory.AppendSegment(current_coordinate, new_data);
        }
        // to prevent a cut of the polyline at the end we need to set the end segment manually
        factory.SetEndSegment(raw_route.segment_end_coordinates.back().target_phantom,
                              raw_route.target_traversed_in_reverse.back(),
                              raw_route.is_via_leg(raw_route.unpacked_path_segments.size()));
    }

    void createGeometry(const osrm::matching::SubMatching &sub,
                        const RouteParameters &route_parameters,
                        const InternalRouteResult &raw_route,
                        DescriptionFactory &factory)
    {
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
    }

    osrm::json::Object submatchingToJSON(const osrm::matching::SubMatching &sub,
                                         const RouteParameters &route_parameters,
                                         const InternalRouteResult &raw_route)
    {
        osrm::json::Object subtrace;
        std::list<SavedCoordinates> coordinate_list;

        if (route_parameters.classify)
        {
            subtrace.values["confidence"] = sub.confidence;
        }

        if (route_parameters.geometry)
        {
            DescriptionFactory factory;

            if(route_parameters.cut)
            {
                createAndCutGeometry(sub, route_parameters, raw_route, factory, coordinate_list);
            }
            else
            {
                createGeometry(sub, route_parameters, raw_route, factory);
            }

            // we need this because we don't run DP
            for (auto &segment : factory.path_description)
            {
                segment.necessary = true;
            }
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

        if(!coordinate_list.empty())
        {
            coordinate_list.clear();
        }

        return subtrace;
    }

    int HandleRequest(const RouteParameters &route_parameters,
                      osrm::json::Object &json_result) final
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

            matchings.values.emplace_back(submatchingToJSON(sub, route_parameters, raw_route));
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
