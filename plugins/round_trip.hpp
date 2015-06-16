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

#ifndef ROUND_TRIP_HPP
#define ROUND_TRIP_HPP

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

template <class DataFacadeT> class RoundTripPlugin final : public BasePlugin
{
  private:
    std::string descriptor_string;
    DataFacadeT *facade;
    std::unique_ptr<SearchEngine<DataFacadeT>> search_engine_ptr;

  public:
    explicit RoundTripPlugin(DataFacadeT *facade)
        : descriptor_string("trip"), facade(facade)
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

        // compute TSP round trip
        InternalRouteResult min_route_nn;
        InternalRouteResult min_route_fi;
        InternalRouteResult min_route_bf;
        std::vector<int> min_loc_permutation_nn(phantom_node_vector.size(), -1);
        std::vector<int> min_loc_permutation_fi(phantom_node_vector.size(), -1);
        std::vector<int> min_loc_permutation_bf(phantom_node_vector.size(), -1);
        TIMER_STOP(tsp_pre);


        //######################### NEAREST NEIGHBOUR ###############################//
        TIMER_START(tsp_nn);
        osrm::tsp::NearestNeighbour(route_parameters, phantom_node_vector, *result_table, min_route_nn, min_loc_permutation_nn);
        search_engine_ptr->shortest_path(min_route_nn.segment_end_coordinates, route_parameters.uturns, min_route_nn);
        TIMER_STOP(tsp_nn);
        SimpleLogger().Write() << "Distance " << min_route_nn.shortest_path_length;
        SimpleLogger().Write() << "Time " << TIMER_MSEC(tsp_nn) + TIMER_MSEC(tsp_pre);

        osrm::json::Array json_loc_permutation_nn;
        json_loc_permutation_nn.values.insert(json_loc_permutation_nn.values.end(), min_loc_permutation_nn.begin(), min_loc_permutation_nn.end());
        json_result.values["nn_loc_permutation"] = json_loc_permutation_nn;
        json_result.values["nn_distance"] = min_route_nn.shortest_path_length;
        json_result.values["nn_runtime"] = TIMER_MSEC(tsp_nn) + TIMER_MSEC(tsp_pre);


        //########################### BRUTE FORCE ####################################//
        if (route_parameters.coordinates.size() < 12) {
            TIMER_START(tsp_bf);
            osrm::tsp::BruteForce(route_parameters, phantom_node_vector, *result_table, min_route_bf, min_loc_permutation_bf);
            search_engine_ptr->shortest_path(min_route_bf.segment_end_coordinates, route_parameters.uturns, min_route_bf);
            TIMER_STOP(tsp_bf);
            SimpleLogger().Write() << "Distance " << min_route_bf.shortest_path_length;
            SimpleLogger().Write() << "Time " << TIMER_MSEC(tsp_bf) + TIMER_MSEC(tsp_pre);

            osrm::json::Array json_loc_permutation_bf;
            json_loc_permutation_bf.values.insert(json_loc_permutation_bf.values.end(), min_loc_permutation_bf.begin(), min_loc_permutation_bf.end());
            json_result.values["bf_loc_permutation"] = json_loc_permutation_bf;
            json_result.values["bf_distance"] = min_route_bf.shortest_path_length;
            json_result.values["bf_runtime"] = TIMER_MSEC(tsp_bf) + TIMER_MSEC(tsp_pre);
        } else {
            json_result.values["bf_distance"] = -1;
            json_result.values["bf_runtime"] = -1;
        }


        //######################## FARTHEST INSERTION ###############################//
        TIMER_START(tsp_fi);
        osrm::tsp::FarthestInsertion(route_parameters, phantom_node_vector, *result_table, min_route_fi, min_loc_permutation_fi);
        search_engine_ptr->shortest_path(min_route_fi.segment_end_coordinates, route_parameters.uturns, min_route_fi);
        TIMER_STOP(tsp_fi);
        SimpleLogger().Write() << "Distance " << min_route_fi.shortest_path_length;
        SimpleLogger().Write() << "Time " << TIMER_MSEC(tsp_fi) + TIMER_MSEC(tsp_pre);

        osrm::json::Array json_loc_permutation_fi;
        json_loc_permutation_fi.values.insert(json_loc_permutation_fi.values.end(), min_loc_permutation_fi.begin(), min_loc_permutation_fi.end());
        json_result.values["fi_loc_permutation"] = json_loc_permutation_fi;
        json_result.values["fi_distance"] = min_route_fi.shortest_path_length;
        json_result.values["fi_runtime"] = TIMER_MSEC(tsp_fi) + TIMER_MSEC(tsp_pre);

        // return geometry result to json
        std::unique_ptr<BaseDescriptor<DataFacadeT>> descriptor;
        descriptor = osrm::make_unique<JSONDescriptor<DataFacadeT>>(facade);

        descriptor->SetConfig(route_parameters);
        descriptor->Run(min_route_fi, json_result);



        return 200;
    }

};

#endif // ROUND_TRIP_HPP
