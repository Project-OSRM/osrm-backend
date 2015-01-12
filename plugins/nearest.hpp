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

#ifndef NEAREST_PLUGIN_H
#define NEAREST_PLUGIN_H

#include "plugin_base.hpp"

#include "../data_structures/phantom_node.hpp"
#include "../util/integer_range.hpp"
#include "../util/json_renderer.hpp"

#include <osrm/json_container.hpp>

#include <string>

/*
 * This Plugin locates the nearest point on a street in the road network for a given coordinate.
 */

template <class DataFacadeT> class NearestPlugin final : public BasePlugin
{
  public:
    explicit NearestPlugin(DataFacadeT *facade) : facade(facade), descriptor_string("nearest") {}

    const std::string GetDescriptor() const override final { return descriptor_string; }

    int HandleRequest(const RouteParameters &route_parameters,
                      osrm::json::Object &json_result) override final
    {
        // check number of parameters
        if (route_parameters.coordinates.empty() ||
            !route_parameters.coordinates.front().is_valid())
        {
            return 400;
        }
        const auto number_of_results = static_cast<std::size_t>(route_parameters.num_results);
        std::vector<PhantomNode> phantom_node_vector;
        facade->IncrementalFindPhantomNodeForCoordinate(route_parameters.coordinates.front(),
                                                        phantom_node_vector,
                                                        static_cast<int>(number_of_results));

        const bool found_coordinate = !phantom_node_vector.empty() &&
                                      phantom_node_vector.front().is_valid();

        const auto output_size = std::min(number_of_results, phantom_node_vector.size());

        if ("pbf" == route_parameters.output_format)
        {
            protobuffer_response::nearest_response nearest_response;
            nearest_response.set_status(207);
            if (found_coordinate)
            {
                nearest_response.set_status(0);
                for (const auto i :
                     osrm::irange<std::size_t>(0, std::min(number_of_results, output_size)))
                {
                    protobuffer_response::named_location location;
                    protobuffer_response::coordinate coordinate;
                    coordinate.set_lat(phantom_node_vector.at(i).location.lat / COORDINATE_PRECISION);
                    coordinate.set_lon(phantom_node_vector.at(i).location.lon / COORDINATE_PRECISION);
                    location.mutable_mapped_coordinate()->CopyFrom(coordinate);
                    std::string temp_string;
                    facade->GetName(phantom_node_vector.at(i).name_id, temp_string);
                    location.set_name(temp_string);
                    nearest_response.add_location()->CopyFrom(location);
                }
            }
            return 200;
        }

        json_result.values["status"] = 207;
        if (found_coordinate)
        {
            json_result.values["status"] = 0;

            JSON::Array results;

            for (const auto i :
                 osrm::irange<std::size_t>(0, std::min(number_of_results, output_size)))
            {
                osrm::json::Array json_coordinate;
                osrm::json::Object result;
                json_coordinate.values.push_back(phantom_node_vector.at(i).location.lat /
                                                 COORDINATE_PRECISION);
                json_coordinate.values.push_back(phantom_node_vector.at(i).location.lon /
                                                 COORDINATE_PRECISION);
                result.values["mapped coordinate"] = json_coordinate;
                std::string temp_string;
                facade->GetName(phantom_node_vector.at(i).name_id, temp_string);
                result.values["name"] = temp_string;
                results.values.push_back(result);
            }
            json_result.values["results"] = results;
        }
        return 200;
    }

  private:
    DataFacadeT *facade;
    std::string descriptor_string;
};

#endif /* NEAREST_PLUGIN_H */
