/*

Copyright (c) 2014, Project OSRM, Dennis Luxen, others
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

#ifndef VIA_ROUTE_PLUGIN_H
#define VIA_ROUTE_PLUGIN_H

#include "plugin_base.hpp"

#include "../algorithms/object_encoder.hpp"
#include "../data_structures/search_engine.hpp"
#include "../descriptors/descriptor_base.hpp"
#include "../descriptors/gpx_descriptor.hpp"
#include "../descriptors/json_descriptor.hpp"
#include "../Util/integer_range.hpp"
#include "../Util/json_renderer.hpp"
#include "../Util/make_unique.hpp"
#include "../Util/simple_logger.hpp"

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

    const std::string GetDescriptor() const final { return descriptor_string; }

    void HandleRequest(const RouteParameters &route_parameters, http::Reply &reply) final
    {
        if (!check_all_coordinates(route_parameters.coordinates))
        {
            reply = http::Reply::StockReply(http::Reply::badRequest);
            return;
        }
        reply.status = http::Reply::ok;

        std::vector<PhantomNode> phantom_node_vector(route_parameters.coordinates.size());
        const bool checksum_OK = (route_parameters.check_sum == facade->GetCheckSum());

        for (const auto i : osrm::irange<std::size_t>(0, route_parameters.coordinates.size()))
        {
            if (checksum_OK && i < route_parameters.hints.size() &&
                !route_parameters.hints[i].empty())
            {
                ObjectEncoder::DecodeFromBase64(route_parameters.hints[i], phantom_node_vector[i]);
                if (phantom_node_vector[i].is_valid(facade->GetNumberOfNodes()))
                {
                    continue;
                }
            }
            facade->IncrementalFindPhantomNodeForCoordinate(route_parameters.coordinates[i],
                                                            phantom_node_vector[i]);
        }

        RawRouteData raw_route;
        auto build_phantom_pairs = [&raw_route] (const PhantomNode &first, const PhantomNode &second)
        {
            raw_route.segment_end_coordinates.emplace_back(PhantomNodes{first, second});
        };
        osrm::for_each_pair(phantom_node_vector, build_phantom_pairs);

        if (route_parameters.alternate_route &&
            1 == raw_route.segment_end_coordinates.size())
        {
            search_engine_ptr->alternative_path(
                raw_route.segment_end_coordinates.front(), raw_route);
        }
        else
        {
            search_engine_ptr->shortest_path(
                raw_route.segment_end_coordinates, route_parameters.uturns, raw_route);
        }

        if (INVALID_EDGE_WEIGHT == raw_route.shortest_path_length)
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

        JSON::Object result;
        descriptor->SetConfig(route_parameters);
        descriptor->Run(raw_route, result);
        descriptor->Render(result, reply.content);
    }
};

#endif // VIA_ROUTE_PLUGIN_H
