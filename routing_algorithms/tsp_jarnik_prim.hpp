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

#ifndef TSP_JARNIK_PRIM_HPP
#define TSP_JARNIK_PRIM_HPP


#include "../data_structures/search_engine.hpp"
#include "../util/string_util.hpp"

#include <osrm/json_container.hpp>

#include <cstdlib>

#include <algorithm>
#include <string>
#include <vector>
#include <limits>
#include <iostream>
#include <queue>
#include "../util/simple_logger.hpp"
#include "../tools/tsp_logs.hpp"

namespace osrm
{
namespace tsp
{

template <typename T>
void PushAdjacentNodes(const std::vector<EdgeWeight> & dist_table,
                       const int& new_node,
                       const int& number_of_locations,
                       const std::vector<bool> & visited,
                       std::priority_queue<int, std::vector<int>, T > & min_heap) {
    for (int i = new_node * number_of_locations; i < (new_node+1) * number_of_locations; ++i) {
        if (dist_table[i] != 0 && !visited[i]) {
            min_heap.push(i);
        }
    }
}


// Finds a MST in a graph. Important: Only works for undirected graphs
void JarnikPrim(const RouteParameters & route_parameters,
                const int number_of_locations,
                const std::vector<EdgeWeight> & dist_table,
                std::vector<int> mst) {

    std::vector<bool> visited (number_of_locations, false);

    int min = std::numeric_limits<int>::max();
    auto min_node = -1;
    for (int i = 0; i < number_of_locations; ++i) {
        if (dist_table[i] != 0 && dist_table[i] < min) {
            min = dist_table[i];
            min_node = i;
        }
    }
    visited[0] = true;
    visited[min_node] = true;
    mst.push_back(min_node);

    auto comp = [dist_table]( const int& a, const int& b ) { return dist_table[a] > dist_table[b]; };
    std::priority_queue<int, std::vector<int>, decltype(comp)> min_heap(comp);

    //add all edges from node 0
    PushAdjacentNodes<decltype(comp)>(dist_table, 0, number_of_locations, visited, min_heap);

    //add all edges from node min_node
    PushAdjacentNodes<decltype(comp)>(dist_table, min_node, number_of_locations, visited, min_heap);


    while(mst.size() < number_of_locations - 1) {
        auto curr = min_heap.top();
        min_heap.pop();
        auto to = curr % number_of_locations;
        if (!visited[to]) {
            visited[to] = true;
            mst.push_back(curr);
            PushAdjacentNodes<decltype(comp)>(dist_table, to, number_of_locations, visited, min_heap);
        }
    }
}

}
}
#endif // TSP_JARNIK_PRIM_HPP