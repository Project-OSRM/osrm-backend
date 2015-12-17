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
    int max_locations_viaroute;

  public:
    explicit ViaRoutePlugin(DataFacadeT *facade, int max_locations_viaroute)
        : descriptor_string("viaroute"), facade(facade),
          max_locations_viaroute(max_locations_viaroute)
    {
        search_engine_ptr = osrm::make_unique<SearchEngine<DataFacadeT>>(facade);

        descriptor_table.emplace("json", 0);
        descriptor_table.emplace("gpx", 1);
        // descriptor_table.emplace("geojson", 2);
    }

    virtual ~ViaRoutePlugin() {}

    const std::string GetDescriptor() const override final { return descriptor_string; }

    Status HandleRequest(const RouteParameters &route_parameters,
                      osrm::json::Object &json_result) override final
    {
        if (max_locations_viaroute > 0 &&
            (static_cast<int>(route_parameters.coordinates.size()) > max_locations_viaroute))
        {
            json_result.values["status_message"] =
                "Number of entries " + std::to_string(route_parameters.coordinates.size()) +
                " is higher than current maximum (" + std::to_string(max_locations_viaroute) + ")";
            return Status::Error;
        }

        if (!check_all_coordinates(route_parameters.coordinates))
        {
            json_result.values["status_message"] = "Invalid coordinates.";
            return Status::Error;
        }

        const auto &input_bearings = route_parameters.bearings;
        if (input_bearings.size() > 0 &&
            route_parameters.coordinates.size() != input_bearings.size())
        {
            json_result.values["status_message"] =
                "Number of bearings does not match number of coordinates.";
            return Status::Error;
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
            const int range = input_bearings.size() > 0
                                  ? (input_bearings[i].second ? *input_bearings[i].second : 10)
                                  : 180;
            phantom_node_pair_list[i] = facade->NearestPhantomNodeWithAlternativeFromBigComponent(
                route_parameters.coordinates[i], bearing, range);
            // we didn't found a fitting node, return error
            if (!phantom_node_pair_list[i].first.is_valid(facade->GetNumberOfNodes()))
            {
                json_result.values["status_message"] =
                    std::string("Could not find a matching segment for coordinate ") +
                    std::to_string(i);
                return Status::NoSegment;
            }
            BOOST_ASSERT(phantom_node_pair_list[i].first.is_valid(facade->GetNumberOfNodes()));
            BOOST_ASSERT(phantom_node_pair_list[i].second.is_valid(facade->GetNumberOfNodes()));
        }

        auto snapped_phantoms = snapPhantomNodes(phantom_node_pair_list);

        InternalRouteResult raw_route;
        auto build_phantom_pairs = [&raw_route](const PhantomNode &first_node,
                                                const PhantomNode &second_node)
        {
            raw_route.segment_end_coordinates.push_back(PhantomNodes{first_node, second_node});
        };
        osrm::for_each_pair(snapped_phantoms, build_phantom_pairs);

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
        if (no_route)
        {
            auto first_component_id = snapped_phantoms.front().component.id;
            auto not_in_same_component =
                std::any_of(snapped_phantoms.begin(), snapped_phantoms.end(),
                            [first_component_id](const PhantomNode &node)
                            {
                                return node.component.id != first_component_id;
                            });
            if (not_in_same_component)
            {
                json_result.values["status_message"] = "Impossible route between points.";
                return Status::EmptyResult;
            }
        }

        return Status::Ok;
    }
};

#endif // VIA_ROUTE_HPP
