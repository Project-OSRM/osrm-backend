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

#ifndef TRIP_HPP
#define TRIP_HPP

#include "plugin_base.hpp"

#include "../algorithms/object_encoder.hpp"
#include "../algorithms/tarjan_scc.hpp"
#include "../algorithms/trip_nearest_neighbour.hpp"
#include "../algorithms/trip_farthest_insertion.hpp"
#include "../algorithms/trip_brute_force.hpp"
#include "../data_structures/search_engine.hpp"
#include "../data_structures/matrix_graph_wrapper.hpp"  // wrapper to use tarjan
                                                        // scc on dist table
#include "../descriptors/descriptor_base.hpp"           // to make json output
#include "../descriptors/json_descriptor.hpp"           // to make json output
#include "../util/make_unique.hpp"
#include "../util/timing_util.hpp"                      // to time runtime
#include "../util/simple_logger.hpp"                    // for logging output
#include "../util/dist_table_wrapper.hpp"               // to access the dist
                                                        // table more easily

#include <osrm/json_container.hpp>
#include <boost/assert.hpp>

#include <cstdlib>
#include <algorithm>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <iterator>

template <class DataFacadeT> class RoundTripPlugin final : public BasePlugin
{
  private:
    std::string descriptor_string;
    DataFacadeT *facade;
    std::unique_ptr<SearchEngine<DataFacadeT>> search_engine_ptr;

  public:
    explicit RoundTripPlugin(DataFacadeT *facade) : descriptor_string("trip"), facade(facade)
    {
        search_engine_ptr = osrm::make_unique<SearchEngine<DataFacadeT>>(facade);
    }

    const std::string GetDescriptor() const override final { return descriptor_string; }

    void GetPhantomNodes(const RouteParameters &route_parameters,
                         PhantomNodeArray &phantom_node_vector)
    {
        const bool checksum_OK = (route_parameters.check_sum == facade->GetCheckSum());

        // find phantom nodes for all input coords
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
                phantom_node_vector[i].erase(std::begin(phantom_node_vector[i]));
            }
            BOOST_ASSERT(phantom_node_vector[i].front().is_valid(facade->GetNumberOfNodes()));
        }
    }

    // Object to hold all strongly connected components (scc) of a graph
    // to access all graphs with component ID i, get the iterators by:
    // auto start = std::begin(scc_component.component) + scc_component.range[i];
    // auto end = std::begin(scc_component.component) + scc_component.range[i+1];
    struct SCC_Component
    {
        // in_component: all NodeIDs sorted by component ID
        // in_range: index where a new component starts
        //
        // example: NodeID 0, 1, 2, 4, 5 are in component 0
        //          NodeID 3, 6, 7, 8    are in component 1
        //          => in_component = [0, 1, 2, 4, 5, 3, 6, 7, 8]
        //          => in_range = [0, 5]
        SCC_Component(std::vector<NodeID> in_component, std::vector<size_t> in_range)
            : component(in_component), range(in_range)
        {
            range.push_back(in_component.size());

            BOOST_ASSERT_MSG(in_component.size() >= in_range.size(),
                             "scc component and its ranges do not match");
            BOOST_ASSERT_MSG(in_component.size() > 0,
                             "there's no scc component");
            BOOST_ASSERT_MSG(*std::max_element(in_range.begin(), in_range.end()) <=
                                 in_component.size(),
                             "scc component ranges are out of bound");
            BOOST_ASSERT_MSG(*std::min_element(in_range.begin(), in_range.end()) >= 0,
                             "invalid scc component range");
            BOOST_ASSERT_MSG(std::is_sorted(std::begin(in_range), std::end(in_range)),
                             "invalid component ranges");
        };

        // constructor to use when whole graph is one single scc
        SCC_Component(std::vector<NodeID> in_component)
                      : component(in_component), range({0, in_component.size()}){};

        std::size_t GetNumberOfComponents() const {
            BOOST_ASSERT_MSG(range.size() > 0,
                             "there's no range");
            return range.size() - 1;
        }

        const std::vector<NodeID> component;
        std::vector<std::size_t> range;
    };

    // takes the number of locations and its distance matrix,
    // identifies and splits the graph in its strongly connected components (scc)
    // and returns an SCC_Component
    SCC_Component SplitUnaccessibleLocations(const std::size_t number_of_locations,
                                             const DistTableWrapper<EdgeWeight> &result_table)
    {

        if (std::find(std::begin(result_table), std::end(result_table), INVALID_EDGE_WEIGHT) == std::end(result_table))
        {
            // whole graph is one scc
            std::vector<NodeID> location_ids(number_of_locations);
            std::iota(std::begin(location_ids), std::end(location_ids), 0);
            return SCC_Component(std::move(location_ids));
        }


        // Run TarjanSCC
        auto wrapper = std::make_shared<MatrixGraphWrapper<EdgeWeight>>(result_table.GetTable(),
                                                                        number_of_locations);
        auto scc = TarjanSCC<MatrixGraphWrapper<EdgeWeight>>(wrapper);
        scc.run();

        const auto number_of_components = scc.get_number_of_components();

        std::vector<std::size_t> range_insertion;
        std::vector<std::size_t> range;
        range_insertion.reserve(number_of_components);
        range.reserve(number_of_components);

        std::vector<NodeID> components(number_of_locations, 0);

        std::size_t prefix = 0;
        for (std::size_t j = 0; j < number_of_components; ++j)
        {
            range_insertion.push_back(prefix);
            range.push_back(prefix);
            prefix += scc.get_component_size(j);
        }

        for (std::size_t i = 0; i < number_of_locations; ++i)
        {
            components[range_insertion[scc.get_component_id(i)]] = i;
            ++range_insertion[scc.get_component_id(i)];
        }

        return SCC_Component(std::move(components), std::move(range));
    }

    void SetLocPermutationOutput(const std::vector<NodeID> &permutation,
                                 osrm::json::Object &json_result)
    {
        osrm::json::Array json_permutation;
        json_permutation.values.insert(std::end(json_permutation.values), std::begin(permutation),
                                       std::end(permutation));
        json_result.values["permutation"] = json_permutation;
    }

    InternalRouteResult ComputeRoute(const PhantomNodeArray &phantom_node_vector,
                                     const RouteParameters &route_parameters,
                                     const std::vector<NodeID> &trip)
    {
        InternalRouteResult min_route;
        // given he final trip, compute total distance and return the route and location permutation
        PhantomNodes viapoint;
        const auto start = std::begin(trip);
        const auto end = std::end(trip);
        for (auto it = start; it != end; ++it)
        {
            const auto from_node = *it;
            // if from_node is the last node, compute the route from the last to the first location
            const auto to_node =
                std::next(it) != end ? *std::next(it) : *start;

            viapoint =
                PhantomNodes{phantom_node_vector[from_node][0], phantom_node_vector[to_node][0]};
            min_route.segment_end_coordinates.emplace_back(viapoint);
        }

        search_engine_ptr->shortest_path(min_route.segment_end_coordinates, route_parameters.uturns,
                                         min_route);

        BOOST_ASSERT_MSG(min_route.shortest_path_length < INVALID_EDGE_WEIGHT, "unroutable route");
        return min_route;
    }

    int HandleRequest(const RouteParameters &route_parameters,
                      osrm::json::Object &json_result) override final
    {
        // check if all inputs are coordinates
        if (!check_all_coordinates(route_parameters.coordinates))
        {
            return 400;
        }

        // get phantom nodes
        PhantomNodeArray phantom_node_vector(route_parameters.coordinates.size());
        GetPhantomNodes(route_parameters, phantom_node_vector);
        const auto number_of_locations = phantom_node_vector.size();

        // compute the distance table of all phantom nodes
        const auto result_table = DistTableWrapper<EdgeWeight>(
            *search_engine_ptr->distance_table(phantom_node_vector), number_of_locations);

        if (result_table.size() == 0)
        {
            return 400;
        }

        const constexpr std::size_t BF_MAX_FEASABLE = 10;
        BOOST_ASSERT_MSG(result_table.size() == number_of_locations * number_of_locations,
                         "Distance Table has wrong size.");

        // get scc components
        SCC_Component scc = SplitUnaccessibleLocations(number_of_locations, result_table);

        using NodeIDIterator = typename std::vector<NodeID>::const_iterator;

        std::vector<std::vector<NodeID>> route_result;
        route_result.reserve(scc.GetNumberOfComponents());
        TIMER_START(TRIP_TIMER);
        // run Trip computation for every SCC
        for (std::size_t k = 0; k < scc.GetNumberOfComponents(); ++k)
        {
            const auto component_size = scc.range[k + 1] - scc.range[k];

            BOOST_ASSERT_MSG(component_size >= 0, "invalid component size");

            if (component_size > 1)
            {
                std::vector<NodeID> scc_route;
                NodeIDIterator start = std::begin(scc.component) + scc.range[k];
                NodeIDIterator end = std::begin(scc.component) + scc.range[k + 1];

                if (component_size < BF_MAX_FEASABLE)
                {
                    scc_route =
                        osrm::trip::BruteForceTrip(start,
                                                   end,
                                                   number_of_locations,
                                                   result_table);
                }
                else
                {
                    scc_route = osrm::trip::FarthestInsertionTrip(start,
                                                                  end,
                                                                  number_of_locations,
                                                                  result_table);
                }

                // use this output if debugging of route is needed:
                // SimpleLogger().Write() << "Route #" << k << ": " << [&scc_route]()
                // {
                //     std::string s = "";
                //     for (auto x : scc_route)
                //     {
                //         s += std::to_string(x) + " ";
                //     }
                //     return s;
                // }();

                route_result.push_back(std::move(scc_route));
            }
            else
            {
                // if component only consists of one node, add it to the result routes
                route_result.emplace_back(scc.component[scc.range[k]]);
            }
        }

        // compute all round trip routes
        std::vector<InternalRouteResult> comp_route;
        comp_route.reserve(route_result.size());
        for (std::size_t r = 0; r < route_result.size(); ++r)
        {
            comp_route.push_back(ComputeRoute(phantom_node_vector, route_parameters, route_result[r]));
        }

        TIMER_STOP(TRIP_TIMER);

        SimpleLogger().Write() << "Trip calculation took: " << TIMER_MSEC(TRIP_TIMER) / 1000. << "s";

        // prepare JSON output
        // create a json object for every trip
        osrm::json::Array trip;
        for (std::size_t i = 0; i < route_result.size(); ++i)
        {
            std::unique_ptr<BaseDescriptor<DataFacadeT>> descriptor = osrm::make_unique<JSONDescriptor<DataFacadeT>>(facade);
            descriptor->SetConfig(route_parameters);

            osrm::json::Object scc_trip;

            // set permutation output
            SetLocPermutationOutput(route_result[i], scc_trip);
            // set viaroute output
            descriptor->Run(comp_route[i], scc_trip);

            trip.values.push_back(std::move(scc_trip));
        }

        json_result.values["trips"] = std::move(trip);



        return 200;
    }
};

#endif // TRIP_HPP
