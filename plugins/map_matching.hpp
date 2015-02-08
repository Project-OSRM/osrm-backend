    /*
        open source routing machine
    Copyright (C) Dennis Luxen, others 2010

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU AFFERO General Public License as published by
the Free Software Foundation; either version 3 of the License, or
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
or see http://www.gnu.org/licenses/agpl.txt.
 */

#ifndef MAP_MATCHING_PLUGIN_H
#define MAP_MATCHING_PLUGIN_H

#include "plugin_base.hpp"

#include "../algorithms/bayes_classifier.hpp"
#include "../algorithms/object_encoder.hpp"
#include "../util/integer_range.hpp"
#include "../data_structures/search_engine.hpp"
#include "../routing_algorithms/map_matching.hpp"
#include "../util/compute_angle.hpp"
#include "../util/simple_logger.hpp"
#include "../util/string_util.hpp"
#include "../descriptors/descriptor_base.hpp"
#include "../descriptors/gpx_descriptor.hpp"
#include "../descriptors/json_descriptor.hpp"

#include <cstdlib>

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

template <class DataFacadeT> class MapMatchingPlugin : public BasePlugin
{
  private:
    std::unordered_map<std::string, unsigned> descriptor_table;
    std::shared_ptr<SearchEngine<DataFacadeT>> search_engine_ptr;

    using ClassifierT = BayesClassifier<LaplaceDistribution, LaplaceDistribution, double>;
    using TraceClassification = ClassifierT::ClassificationT;

  public:
    MapMatchingPlugin(DataFacadeT *facade)
    : descriptor_string("match")
    , facade(facade)
    // the values where derived from fitting a laplace distribution
    // to the values of manually classified traces
    , classifier(LaplaceDistribution(0.0057154021891018675, 0.020294704891166186),
                 LaplaceDistribution(0.11467696742821254, 0.49918444000368756),
                 0.7977883096366508) // valid apriori probability
    {
        descriptor_table.emplace("json", 0);
        descriptor_table.emplace("gpx", 1);
        // descriptor_table.emplace("geojson", 2);
        //
        search_engine_ptr = std::make_shared<SearchEngine<DataFacadeT>>(facade);
    }

    virtual ~MapMatchingPlugin() { search_engine_ptr.reset(); }

    const std::string GetDescriptor() const final { return descriptor_string; }

    TraceClassification classify(double trace_length, const std::vector<PhantomNode>& matched_nodes, int removed_points) const
    {
        double matching_length = 0;
        for (const auto current_node : osrm::irange<std::size_t>(0, matched_nodes.size()-1))
        {
                matching_length += coordinate_calculation::great_circle_distance(
                    matched_nodes[current_node].location,
                    matched_nodes[current_node + 1].location);
        }
        double distance_feature = -std::log(trace_length / matching_length);

        auto label_with_confidence =  classifier.classify(distance_feature);

        // "second stage classifier": if we need to remove points there is something fishy
        if (removed_points > 0)
        {
            label_with_confidence.first = ClassifierT::ClassLabel::NEGATIVE;
            label_with_confidence.second = 1.0;
        }

        return label_with_confidence;
    }

    bool get_candiates(const std::vector<FixedPointCoordinate>& input_coords, double& trace_length, Matching::CandidateLists& candidates_lists)
    {
        double last_distance = coordinate_calculation::great_circle_distance(
            input_coords[0],
            input_coords[1]);
        trace_length = 0;
        for (const auto current_coordinate : osrm::irange<std::size_t>(0, input_coords.size()))
        {
            bool allow_uturn = false;
            if (0 < current_coordinate)
            {
                last_distance = coordinate_calculation::great_circle_distance(
                    input_coords[current_coordinate - 1],
                    input_coords[current_coordinate]);
                trace_length += last_distance;
            }

            if (input_coords.size()-1 > current_coordinate && 0 < current_coordinate)
            {
                double turn_angle = ComputeAngle::OfThreeFixedPointCoordinates(
                                                    input_coords[current_coordinate-1],
                                                    input_coords[current_coordinate],
                                                    input_coords[current_coordinate+1]);

                // sharp turns indicate a possible uturn
                if (turn_angle < 100.0 || turn_angle > 260.0)
                {
                    allow_uturn = true;
                }
            }

            std::vector<std::pair<PhantomNode, double>> candidates;
            if (!facade->IncrementalFindPhantomNodeForCoordinateWithMaxDistance(
                    input_coords[current_coordinate],
                    candidates,
                    last_distance/2.0,
                    5,
                    20))
            {
                return false;
            }

            if (allow_uturn)
            {
                candidates_lists.push_back(candidates);
            }
            else
            {
                unsigned compact_size = candidates.size();
                for (const auto i : osrm::irange(0u, compact_size))
                {
                    // Split edge if it is bidirectional and append reverse direction to end of list
                    if (candidates[i].first.forward_node_id != SPECIAL_NODEID
                     && candidates[i].first.reverse_node_id != SPECIAL_NODEID)
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

    int HandleRequest(const RouteParameters &route_parameters, JSON::Object &json_result) final
    {
        // check number of parameters
        if (!check_all_coordinates(route_parameters.coordinates))
        {
            return 400;
        }

        double trace_length;
        Matching::CandidateLists candidates_lists;
        const auto& input_coords = route_parameters.coordinates;
        bool found_candidates = get_candiates(input_coords, trace_length, candidates_lists);
        if (!found_candidates)
        {
            return 400;
        }

        // call the actual map matching
        JSON::Object debug_info;
        std::vector<PhantomNode> matched_nodes;
        search_engine_ptr->map_matching(candidates_lists, input_coords, matched_nodes, debug_info);

        // classify result
        TraceClassification classification = classify(trace_length, matched_nodes, input_coords.size() - matched_nodes.size());
        if (classification.first == ClassifierT::ClassLabel::POSITIVE)
        {
            json_result.values["confidence"] = classification.second;
        }
        else
        {
            json_result.values["confidence"] = 1-classification.second;
        }

        // run shortest path routing to obtain geometry
        InternalRouteResult raw_route;
        PhantomNodes current_phantom_node_pair;
        for (unsigned i = 0; i < matched_nodes.size() - 1; ++i)
        {
            current_phantom_node_pair.source_phantom = matched_nodes[i];
            current_phantom_node_pair.target_phantom = matched_nodes[i + 1];
            raw_route.segment_end_coordinates.emplace_back(current_phantom_node_pair);
        }

        if (2 > matched_nodes.size())
        {
            return 400;
        }

        search_engine_ptr->shortest_path(
            raw_route.segment_end_coordinates,
            std::vector<bool>(raw_route.segment_end_coordinates.size(), true),
            raw_route);

        DescriptorConfig descriptor_config;

        auto iter = descriptor_table.find(route_parameters.output_format);
        unsigned descriptor_type = (iter != descriptor_table.end() ? iter->second : 0);

        descriptor_config.zoom_level = route_parameters.zoom_level;
        descriptor_config.instructions = route_parameters.print_instructions;
        descriptor_config.geometry = route_parameters.geometry;
        descriptor_config.encode_geometry = route_parameters.compression;

        std::shared_ptr<BaseDescriptor<DataFacadeT>> descriptor;
        switch (descriptor_type)
        {
        // case 0:
        //     descriptor = std::make_shared<JSONDescriptor<DataFacadeT>>();
        //     break;
        case 1:
            descriptor = std::make_shared<GPXDescriptor<DataFacadeT>>(facade);
            break;
        // case 2:
        //      descriptor = std::make_shared<GEOJSONDescriptor<DataFacadeT>>();
        //      break;
        default:
            descriptor = std::make_shared<JSONDescriptor<DataFacadeT>>(facade);
            break;
        }

        descriptor->SetConfig(descriptor_config);
        descriptor->Run(raw_route, json_result);

        json_result.values["debug"] = debug_info;

        return 200;
    }

  private:
    std::string descriptor_string;
    DataFacadeT *facade;
    ClassifierT classifier;
};

#endif /* MAP_MATCHING_PLUGIN_H */
