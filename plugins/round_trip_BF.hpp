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

#ifndef ROUND_TRIP_BF_HPP
#define ROUND_TRIP_BF_HPP

#include "plugin_base.hpp"

#include "../algorithms/object_encoder.hpp"
#include "../routing_algorithms/tsp_nearest_neighbour.hpp"
#include "../routing_algorithms/tsp_farthest_insertion.hpp"
#include "../routing_algorithms/tsp_brute_force.hpp"
#include "../data_structures/query_edge.hpp"
#include "../data_structures/search_engine.hpp"
#include "../descriptors/descriptor_base.hpp"
#include "../descriptors/json_descriptor.hpp"
#include "../util/json_renderer.hpp"
#include "../util/make_unique.hpp"
#include "../util/string_util.hpp"
#include "../util/timing_util.hpp"
#include "../util/simple_logger.hpp"

#include <osrm/json_container.hpp>

#include <cstdlib>
#include <algorithm>
#include <memory>
#include <unordered_map>
#include <string>
#include <vector>
#include <limits>

template <class DataFacadeT> class RoundTripPluginBF final : public BasePlugin
{
  private:
    std::string descriptor_string;
    DataFacadeT *facade;
    std::unique_ptr<SearchEngine<DataFacadeT>> search_engine_ptr;

  public:
    explicit RoundTripPluginBF(DataFacadeT *facade)
        : descriptor_string("tripBF"), facade(facade)
    {
        search_engine_ptr = osrm::make_unique<SearchEngine<DataFacadeT>>(facade);
    }

    const std::string GetDescriptor() const override final { return descriptor_string; }

    int HandleRequest(const RouteParameters &route_parameters,
                      osrm::json::Object &json_result) override final
    {
        TIMER_START(tsp_pre);
        // check if all inputs are coordinates
        if (!check_all_coordinates(route_parameters.coordinates))
        {
            return 400;
        }
        const bool checksum_OK = (route_parameters.check_sum == facade->GetCheckSum());

        // find phantom nodes for all input coords
        PhantomNodeArray phantom_node_vector(route_parameters.coordinates.size());
        for (const auto i : osrm::irange<std::size_t>(0, route_parameters.coordinates.size()))
        {
            // if client hints are helpful, encode hints
            if (checksum_OK && i < route_parameters.hints.size() &&
                !route_parameters.hints[i].empty())
            {
                PhantomNode current_phantom_node;
                ObjectEncoder::DecodeFromBase64(route_parameters.hints[i], current_phantom_node);
                if (current_phantom_node.is_valid(facade->GetNumberOfNodes()))
                {
                    phantom_node_vector[i].emplace_back(std::move(current_phantom_node));
                    continue;
                }
            }
            facade->IncrementalFindPhantomNodeForCoordinate(route_parameters.coordinates[i],
                                                            phantom_node_vector[i], 1);
            if (phantom_node_vector[i].size() > 1)
            {
                phantom_node_vector[i].erase(phantom_node_vector[i].begin());
            }
            BOOST_ASSERT(phantom_node_vector[i].front().is_valid(facade->GetNumberOfNodes()));
        }

        // compute the distance table of all phantom nodes
        const std::shared_ptr<std::vector<EdgeWeight>> result_table =
            search_engine_ptr->distance_table(phantom_node_vector);

        if (!result_table)
        {
            return 400;
        }

        auto number_of_locations = phantom_node_vector.size();
        const auto maxint = std::numeric_limits<int>::max();

        //////////////////////////////////// DELETE UNACCESSIBLE LOCATIONS /////////////////////////////////////////

        if (*std::max_element(result_table->begin(), result_table->end()) == maxint) {
            const int half = number_of_locations / 2;
            std::vector<int> to_delete;

            for (int i = number_of_locations - 1; i >= 0; --i) {
                // if the location is unaccessible by most of the other locations, remember the location
                if (std::count(result_table->begin() + i * number_of_locations, result_table->begin() + (i+1) * number_of_locations, maxint) > half) {
                    to_delete.push_back(i);
                }
            }
            //delete all unaccessible locations
            for (int k = 0; k < to_delete.size(); ++k) {
                // delete its row
                result_table->erase(result_table->begin() + to_delete[k] * number_of_locations, result_table->begin() + (to_delete[k]+1) * number_of_locations);
                --number_of_locations;
                // delete its column
                for (int j = 0; j < number_of_locations; ++j) {
                    result_table->erase(result_table->begin() + j * number_of_locations + to_delete[k]);
                }
            }
        }
        //todo: delete
        if (*std::max_element(result_table->begin(), result_table->end()) == maxint) {
            SimpleLogger().Write() << "SOMETHING WENT WRONG";
        }

        // compute TSP round trip
        InternalRouteResult min_route;
        std::vector<int> min_loc_permutation(phantom_node_vector.size(), -1);
        TIMER_STOP(tsp_pre);

        //########################### BRUTE FORCE ####################################//
        if (route_parameters.coordinates.size() < 11) {
            TIMER_START(tsp);
            osrm::tsp::BruteForceTSP(phantom_node_vector, *result_table, min_route, min_loc_permutation);
            search_engine_ptr->shortest_path(min_route.segment_end_coordinates, route_parameters.uturns, min_route);
            TIMER_STOP(tsp);

            BOOST_ASSERT(min_route.segment_end_coordinates.size() == route_parameters.coordinates.size());

            osrm::json::Array json_loc_permutation;
            json_loc_permutation.values.insert(json_loc_permutation.values.end(), min_loc_permutation.begin(), min_loc_permutation.end());
            json_result.values["loc_permutation"] = json_loc_permutation;
            json_result.values["distance"] = min_route.shortest_path_length;
            SimpleLogger().Write() << "BF GEOM DISTANCE " << min_route.shortest_path_length;
            json_result.values["runtime"] = TIMER_MSEC(tsp);
        } else {
            json_result.values["distance"] = -1;
            json_result.values["runtime"] = -1;
        }
        // return geometry result to json

        std::unique_ptr<BaseDescriptor<DataFacadeT>> descriptor;
        descriptor = osrm::make_unique<JSONDescriptor<DataFacadeT>>(facade);

        descriptor->SetConfig(route_parameters);
        descriptor->Run(min_route, json_result);



        return 200;
    }

};

#endif // ROUND_TRIP_BF_HPP
