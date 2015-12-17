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

#ifndef DISTANCE_TABLE_HPP
#define DISTANCE_TABLE_HPP

#include "plugin_base.hpp"

#include "../algorithms/object_encoder.hpp"
#include "../data_structures/query_edge.hpp"
#include "../data_structures/search_engine.hpp"
#include "../descriptors/descriptor_base.hpp"
#include "../util/json_renderer.hpp"
#include "../util/make_unique.hpp"
#include "../util/string_util.hpp"
#include "../util/timing_util.hpp"

#include <osrm/json_container.hpp>

#include <cstdlib>

#include <algorithm>
#include <memory>
#include <unordered_map>
#include <string>
#include <vector>

template <class DataFacadeT> class DistanceTablePlugin final : public BasePlugin
{
  private:
    std::unique_ptr<SearchEngine<DataFacadeT>> search_engine_ptr;
    int max_locations_distance_table;

  public:
    explicit DistanceTablePlugin(DataFacadeT *facade, const int max_locations_distance_table)
        : max_locations_distance_table(max_locations_distance_table), descriptor_string("table"),
          facade(facade)
    {
        search_engine_ptr = osrm::make_unique<SearchEngine<DataFacadeT>>(facade);
    }

    virtual ~DistanceTablePlugin() {}

    const std::string GetDescriptor() const override final { return descriptor_string; }

    int HandleRequest(const RouteParameters &route_parameters,
                      osrm::json::Object &json_result) override final
    {
        if (!check_all_coordinates(route_parameters.coordinates))
        {
            json_result.values["status_message"] = "Coordinates are invalid.";
            return 400;
        }

        const auto &input_bearings = route_parameters.bearings;
        if (input_bearings.size() > 0 &&
            route_parameters.coordinates.size() != input_bearings.size())
        {
            json_result.values["status_message"] =
                "Number of bearings does not match number of coordinates.";
            return 400;
        }

        auto number_of_sources =
            std::count_if(route_parameters.is_source.begin(), route_parameters.is_source.end(),
                          [](const bool is_source)
                          {
                              return is_source;
                          });
        auto number_of_destination =
            std::count_if(route_parameters.is_destination.begin(),
                          route_parameters.is_destination.end(), [](const bool is_destination)
                          {
                              return is_destination;
                          });

        if (max_locations_distance_table > 0 &&
            (number_of_sources * number_of_destination >
             max_locations_distance_table * max_locations_distance_table))
        {
            json_result.values["status_message"] =
                "Number of entries " + std::to_string(number_of_sources * number_of_destination) +
                " is higher than current maximum (" +
                std::to_string(max_locations_distance_table * max_locations_distance_table) + ")";
            return 400;
        }

        const bool checksum_OK = (route_parameters.check_sum == facade->GetCheckSum());

        std::vector<PhantomNodePair> phantom_node_source_vector(number_of_sources);
        std::vector<PhantomNodePair> phantom_node_target_vector(number_of_destination);
        auto phantom_node_source_out_iter = phantom_node_source_vector.begin();
        auto phantom_node_target_out_iter = phantom_node_target_vector.begin();
        for (const auto i : osrm::irange<std::size_t>(0u, route_parameters.coordinates.size()))
        {
            if (checksum_OK && i < route_parameters.hints.size() &&
                !route_parameters.hints[i].empty())
            {
                PhantomNode current_phantom_node;
                ObjectEncoder::DecodeFromBase64(route_parameters.hints[i], current_phantom_node);
                if (current_phantom_node.is_valid(facade->GetNumberOfNodes()))
                {
                    if (route_parameters.is_source[i])
                    {
                        *phantom_node_source_out_iter =
                            std::make_pair(current_phantom_node, current_phantom_node);
                        if (route_parameters.is_destination[i])
                        {
                            *phantom_node_target_out_iter = *phantom_node_source_out_iter;
                            phantom_node_target_out_iter++;
                        }
                        phantom_node_source_out_iter++;
                    }
                    else
                    {
                        BOOST_ASSERT(route_parameters.is_destination[i] &&
                                     !route_parameters.is_source[i]);
                        *phantom_node_target_out_iter =
                            std::make_pair(current_phantom_node, current_phantom_node);
                        phantom_node_target_out_iter++;
                    }
                    continue;
                }
            }
            const int bearing = input_bearings.size() > 0 ? input_bearings[i].first : 0;
            const int range = input_bearings.size() > 0
                                  ? (input_bearings[i].second ? *input_bearings[i].second : 10)
                                  : 180;
            if (route_parameters.is_source[i])
            {
                *phantom_node_source_out_iter =
                    facade->NearestPhantomNodeWithAlternativeFromBigComponent(
                        route_parameters.coordinates[i], bearing, range);
                // we didn't found a fitting node, return error
                if (!phantom_node_source_out_iter->first.is_valid(facade->GetNumberOfNodes()))
                {
                    json_result.values["status_message"] =
                        std::string("Could not find matching road for via ") + std::to_string(i);
                    return 400;
                }

                if (route_parameters.is_destination[i])
                {
                    *phantom_node_target_out_iter = *phantom_node_source_out_iter;
                    phantom_node_target_out_iter++;
                }
                phantom_node_source_out_iter++;
            }
            else
            {
                BOOST_ASSERT(route_parameters.is_destination[i] && !route_parameters.is_source[i]);

                *phantom_node_target_out_iter =
                    facade->NearestPhantomNodeWithAlternativeFromBigComponent(
                        route_parameters.coordinates[i], bearing, range);
                // we didn't found a fitting node, return error
                if (!phantom_node_target_out_iter->first.is_valid(facade->GetNumberOfNodes()))
                {
                    json_result.values["status_message"] =
                        std::string("Could not find matching road for via ") + std::to_string(i);
                    return 400;
                }
                phantom_node_target_out_iter++;
            }
        }

        BOOST_ASSERT((phantom_node_source_out_iter - phantom_node_source_vector.begin()) ==
                     number_of_sources);
        BOOST_ASSERT((phantom_node_target_out_iter - phantom_node_target_vector.begin()) ==
                     number_of_destination);

        // FIXME we should clear phantom_node_source_vector and phantom_node_target_vector after
        // this
        auto snapped_source_phantoms = snapPhantomNodes(phantom_node_source_vector);
        auto snapped_target_phantoms = snapPhantomNodes(phantom_node_target_vector);

        auto result_table =
            search_engine_ptr->distance_table(snapped_source_phantoms, snapped_target_phantoms);

        if (!result_table)
        {
            json_result.values["status_message"] = "No distance table found.";
            return 400;
        }

        osrm::json::Array matrix_json_array;
        for (const auto row : osrm::irange<std::size_t>(0, number_of_sources))
        {
            osrm::json::Array json_row;
            auto row_begin_iterator = result_table->begin() + (row * number_of_destination);
            auto row_end_iterator = result_table->begin() + ((row + 1) * number_of_destination);
            json_row.values.insert(json_row.values.end(), row_begin_iterator, row_end_iterator);
            matrix_json_array.values.push_back(json_row);
        }
        json_result.values["distance_table"] = matrix_json_array;

        osrm::json::Array target_coord_json_array;
        for (const auto &phantom : snapped_target_phantoms)
        {
            osrm::json::Array json_coord;
            json_coord.values.push_back(phantom.location.lat / COORDINATE_PRECISION);
            json_coord.values.push_back(phantom.location.lon / COORDINATE_PRECISION);
            target_coord_json_array.values.push_back(json_coord);
        }
        json_result.values["destination_coordinates"] = target_coord_json_array;
        osrm::json::Array source_coord_json_array;
        for (const auto &phantom : snapped_source_phantoms)
        {
            osrm::json::Array json_coord;
            json_coord.values.push_back(phantom.location.lat / COORDINATE_PRECISION);
            json_coord.values.push_back(phantom.location.lon / COORDINATE_PRECISION);
            source_coord_json_array.values.push_back(json_coord);
        }
        json_result.values["source_coordinates"] = source_coord_json_array;
        return 200;
    }

  private:
    std::string descriptor_string;
    DataFacadeT *facade;
};

#endif // DISTANCE_TABLE_HPP
