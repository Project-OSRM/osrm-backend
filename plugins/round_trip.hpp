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
        std::shared_ptr<std::vector<EdgeWeight>> result_table =
            search_engine_ptr->distance_table(phantom_node_vector);

        if (!result_table)
        {
            return 400;
        }

        //////////////////////////////////////////////////////////////////////////////////////////////////
        // START GREEDY NEAREST NEIGHBOUR HERE
        // 1. grab a random location and mark as starting point
        // 2. find the nearest unvisited neighbour, set it as the current location and mark as visited
        // 3. repeat 2 until there is no unvisited location
        // 4. return route back to starting point
        // 5. compute route
        // 6. DONE!
        //////////////////////////////////////////////////////////////////////////////////////////////////

        const auto number_of_locations = phantom_node_vector.size();
        SimpleLogger().Write() << number_of_locations;
        InternalRouteResult min_route;
        std::vector<int> min_loc_permutation;
        min_route.shortest_path_length = std::numeric_limits<int>::max();

        std::vector<bool> lonely_island(number_of_locations, false);

        int count_unreachables;
        // SET RANDOM START LOCATION
        for(int start_node = 0; start_node < number_of_locations; ++start_node)
        {
            // check whether this start node is a lonely island
            if (lonely_island[start_node])
                continue;
            count_unreachables = 0;
            auto start_dist_begin = result_table->begin() + (start_node * number_of_locations);
            auto start_dist_end = result_table->begin() + ((start_node + 1) * number_of_locations);
            for (auto it2 = start_dist_begin; it2 != start_dist_end; ++it2) {
                SimpleLogger().Write() << *it2;
                if (*it2 == 0 || *it2 == std::numeric_limits<int>::max()) {
                    ++count_unreachables;
                }
            }
            if (count_unreachables >= number_of_locations) {
                lonely_island[start_node] = true;
                continue;
            }

            int curr_node = start_node;   
            InternalRouteResult raw_route;
            std::vector<int> loc_permutation(number_of_locations, -1);
            loc_permutation[start_node] = 0;
            std::vector<bool> visited(number_of_locations, false);
            visited[start_node] = true;

            PhantomNodes subroute;
            // 3. REPEAT FOR EVERY UNVISITED NODE
            for(int stopover = 1; stopover < number_of_locations; ++stopover)
            {
                auto row_begin_iterator = result_table->begin() + (curr_node * number_of_locations);
                auto row_end_iterator = result_table->begin() + ((curr_node + 1) * number_of_locations);
                int min_dist = std::numeric_limits<int>::max();
                int min_id = -1;

                // 2. FIND NEAREST NEIGHBOUR
                for (auto it = row_begin_iterator; it != row_end_iterator; ++it) {
                    auto index = std::distance(row_begin_iterator, it); 
                    if (!lonely_island[index] && !visited[index] && *it < min_dist)
                    {
                        min_dist = *it;
                        min_id = index;
                    }
                }
                if (min_id == -1)
                {
                    for(int loc = 0; loc < visited.size(); ++loc) {
                        if (!visited[loc]) {
                            lonely_island[loc] = true;
                        }
                    }
                    break;
                }
                else
                {
                    loc_permutation[min_id] = stopover;
                    visited[min_id] = true;
                    subroute = PhantomNodes{phantom_node_vector[curr_node][0], phantom_node_vector[min_id][0]};
                    raw_route.segment_end_coordinates.emplace_back(subroute);
                    curr_node = min_id;
                }
            }

            // 4. ROUTE BACK TO STARTING POINT
            subroute = PhantomNodes{raw_route.segment_end_coordinates.back().target_phantom, phantom_node_vector[start_node][0]};
            raw_route.segment_end_coordinates.emplace_back(subroute);

            // 5. COMPUTE ROUTE
            search_engine_ptr->shortest_path(raw_route.segment_end_coordinates, route_parameters.uturns, raw_route);
            // SimpleLogger().Write() << "Route starting at " << start_node << " with length " << raw_route.shortest_path_length;
            if (raw_route.shortest_path_length < min_route.shortest_path_length) {
                min_route = raw_route;
                min_loc_permutation = loc_permutation;
            }
        }
        // SimpleLogger().Write() << "Shortest route has length "  << min_route.shortest_path_length;
        for(int loc = 0; loc < lonely_island.size(); ++loc) {
            if (lonely_island[loc]) {
                SimpleLogger().Write() << "Loc " << loc << " is a lonely island.";
            }
        }

        // return result to json
        std::unique_ptr<BaseDescriptor<DataFacadeT>> descriptor;
        descriptor = osrm::make_unique<JSONDescriptor<DataFacadeT>>(facade);
        
        descriptor->SetConfig(route_parameters);
        descriptor->Run(min_route, json_result);

        osrm::json::Array json_loc_permutation;
        json_loc_permutation.values.insert(json_loc_permutation.values.end(), min_loc_permutation.begin(), min_loc_permutation.end());
        json_result.values["loc_permutation"] = json_loc_permutation;

        return 200;
    }

};

#endif // ROUND_TRIP_HPP
