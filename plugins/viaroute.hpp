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

#ifndef VIA_ROUTE_HPP
#define VIA_ROUTE_HPP

#include "plugin_base.hpp"

#include "../algorithms/object_encoder.hpp"
#include "../data_structures/search_engine.hpp"
#include "../descriptors/descriptor_base.hpp"
#include "../descriptors/gpx_descriptor.hpp"
#include "../descriptors/json_descriptor.hpp"
#include "../util/integer_range.hpp"
#include "../util/json_renderer.hpp"
#include "../util/make_unique.hpp"
#include "../util/simple_logger.hpp"
#include "../util/timing_util.hpp"

#include <osrm/json_container.hpp>

#include <cstdlib>

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

template <class DataFacadeT> class ViaRoutePlugin final : public BasePlugin
{
  private:
    DescriptorTable descriptor_table;
    std::string descriptor_string;
    std::unique_ptr<SearchEngine<DataFacadeT>> search_engine_ptr;
    DataFacadeT *facade;

  public:
    explicit ViaRoutePlugin(DataFacadeT *facade) : descriptor_string("viaroute"), facade(facade)
    {
        search_engine_ptr = osrm::make_unique<SearchEngine<DataFacadeT>>(facade);

        descriptor_table.emplace("json", 0);
        descriptor_table.emplace("gpx", 1);
        // descriptor_table.emplace("geojson", 2);
    }

    virtual ~ViaRoutePlugin() {}

    const std::string GetDescriptor() const override final { return descriptor_string; }

    int HandleRequest(const RouteParameters &route_parameters,
                      osrm::json::Object &json_result) override final
    {
        if (!check_all_coordinates(route_parameters.coordinates))
        {
            return 400;
        }

        const auto &input_bearings = route_parameters.bearings;
        if (input_bearings.size() > 0 && route_parameters.coordinates.size() != input_bearings.size())
        {
            json_result.values["status"] = "Number of bearings does not match number of coordinates .";
            return 400;
        }

        std::vector<PhantomNodePair> phantom_node_pair_list(route_parameters.coordinates.size());
        const bool checksum_OK = (route_parameters.check_sum == facade->GetCheckSum());

        for (const auto i : osrm::irange<std::size_t>(0, route_parameters.coordinates.size()))
        {
            if (checksum_OK && i < route_parameters.hints.size() &&
                !route_parameters.hints[i].empty())
            {
                ObjectEncoder::DecodeFromBase64(route_parameters.hints[i],
                                                phantom_node_pair_list[i].first);
                if (phantom_node_pair_list[i].first.is_valid(facade->GetNumberOfNodes()))
                {
                    continue;
                }
            }
            const int bearing = input_bearings.size() > 0 ? input_bearings[i].first : 0;
            const int range = input_bearings.size() > 0 ? (input_bearings[i].second?*input_bearings[i].second:10) : 180;
            phantom_node_pair_list[i] = facade->NearestPhantomNodeWithAlternativeFromBigComponent(route_parameters.coordinates[i], bearing, range);
            // we didn't found a fitting node, return error
            if (!phantom_node_pair_list[i].first.is_valid(facade->GetNumberOfNodes()))
            {
                json_result.values["status_message"] = std::string("Could not find matching road for via ") + std::to_string(i);
                return 400;
            }
            BOOST_ASSERT(phantom_node_pair_list[i].first.is_valid(facade->GetNumberOfNodes()));
            BOOST_ASSERT(phantom_node_pair_list[i].second.is_valid(facade->GetNumberOfNodes()));
        }

        const auto check_component_id_is_tiny = [](const PhantomNodePair &phantom_pair)
        {
            return phantom_pair.first.component.is_tiny;
        };

        const bool every_phantom_is_in_tiny_cc =
            std::all_of(std::begin(phantom_node_pair_list), std::end(phantom_node_pair_list),
                        check_component_id_is_tiny);

        // are all phantoms from a tiny cc?
        const auto check_all_in_same_component = [](const std::vector<PhantomNodePair> &nodes)
        {
            const auto component_id = nodes.front().first.component.id;

            return std::all_of(std::begin(nodes), std::end(nodes),
                               [component_id](const PhantomNodePair &phantom_pair)
                               {
                                   return component_id == phantom_pair.first.component.id;
                               });
        };

        auto swap_phantom_from_big_cc_into_front = [](PhantomNodePair &phantom_pair)
        {
            if (phantom_pair.first.component.is_tiny && phantom_pair.second.is_valid() && !phantom_pair.second.component.is_tiny)
            {
                using namespace std;
                swap(phantom_pair.first, phantom_pair.second);
            }
        };

        auto all_in_same_component = check_all_in_same_component(phantom_node_pair_list);

        // this case is true if we take phantoms from the big CC
        if (every_phantom_is_in_tiny_cc && !all_in_same_component)
        {
            std::for_each(std::begin(phantom_node_pair_list), std::end(phantom_node_pair_list),
                          swap_phantom_from_big_cc_into_front);

            // update check with new component ids
            all_in_same_component = check_all_in_same_component(phantom_node_pair_list);
        }

        InternalRouteResult raw_route;
        auto build_phantom_pairs =
            [&raw_route](const PhantomNodePair &first_pair, const PhantomNodePair &second_pair)
        {
            raw_route.segment_end_coordinates.emplace_back(
                PhantomNodes{first_pair.first, second_pair.first});
        };
        osrm::for_each_pair(phantom_node_pair_list, build_phantom_pairs);

        if (1 == raw_route.segment_end_coordinates.size())
        {
            if (route_parameters.alternate_route)
            {
              search_engine_ptr->alternative_path(raw_route.segment_end_coordinates.front(),
                                                  raw_route);
            }
            else
            {
                search_engine_ptr->direct_shortest_path(raw_route.segment_end_coordinates,
                                                        route_parameters.uturns, raw_route);
            }
        }
        else
        {
            search_engine_ptr->shortest_path(raw_route.segment_end_coordinates,
                                             route_parameters.uturns, raw_route);
        }

        bool no_route = INVALID_EDGE_WEIGHT == raw_route.shortest_path_length;

        if (no_route)
        {
            SimpleLogger().Write(logDEBUG) << "Error occurred, single path not found";
        }

        std::unique_ptr<BaseDescriptor<DataFacadeT>> descriptor;
        switch (descriptor_table.get_id(route_parameters.output_format))
        {
        case 1:
            descriptor = osrm::make_unique<GPXDescriptor<DataFacadeT>>(facade);
            break;
        // case 2:
        //      descriptor = osrm::make_unique<GEOJSONDescriptor<DataFacadeT>>();
        //      break;
        default:
            descriptor = osrm::make_unique<JSONDescriptor<DataFacadeT>>(facade);
            break;
        }

        descriptor->SetConfig(route_parameters);
        descriptor->Run(raw_route, json_result);

        // we can only know this after the fact, different SCC ids still
        // allow for connection in one direction.
        if (!all_in_same_component && no_route)
        {
            json_result.values["status_message"] = "Impossible route between points.";
            return 400;
        }

        return 200;
    }
};

#endif // VIA_ROUTE_HPP
