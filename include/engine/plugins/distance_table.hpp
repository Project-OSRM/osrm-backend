#ifndef DISTANCE_TABLE_HPP
#define DISTANCE_TABLE_HPP

#include "engine/plugins/plugin_base.hpp"

#include "engine/object_encoder.hpp"
#include "engine/search_engine.hpp"
#include "util/make_unique.hpp"
#include "util/string_util.hpp"
#include "osrm/json_container.hpp"

#include <cstdlib>

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

namespace osrm
{
namespace engine
{
namespace plugins
{

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
        search_engine_ptr = util::make_unique<SearchEngine<DataFacadeT>>(facade);
    }

    virtual ~DistanceTablePlugin() {}

    const std::string GetDescriptor() const override final { return descriptor_string; }

    Status HandleRequest(const RouteParameters &route_parameters,
                         util::json::Object &json_result) override final
    {
        if (!check_all_coordinates(route_parameters.coordinates))
        {
            json_result.values["status_message"] = "Coordinates are invalid";
            return Status::Error;
        }

        const auto &input_bearings = route_parameters.bearings;
        if (input_bearings.size() > 0 &&
            route_parameters.coordinates.size() != input_bearings.size())
        {
            json_result.values["status_message"] =
                "Number of bearings does not match number of coordinates";
            return Status::Error;
        }

        // The check_all_coordinates guard above makes sure we have at least 2 coordinates.
        // This guard makes sure is_source, is_destination, coordinates are parallel arrays.
        if (route_parameters.is_source.size() != route_parameters.coordinates.size() ||
            route_parameters.is_destination.size() != route_parameters.coordinates.size())
        {
            json_result.values["status_message"] =
                "Number of sources and destinations does not match number of coordinates";
            return Status::Error;
        }

        const auto number_of_sources = std::count(route_parameters.is_source.begin(), //
                                                  route_parameters.is_source.end(), true);
        const auto number_of_destination = std::count(route_parameters.is_destination.begin(), //
                                                      route_parameters.is_destination.end(), true);

        // At this point we know that we
        //  - have at least n=2 coordinates and
        //  - have n booleans in is_source and n booleans in is_destination
        //  This guard makes sure we have at least one source and one target.
        if (number_of_sources < 1 || number_of_destination < 1)
        {
            json_result.values["status_message"] =
                "At least one source and one destination required";
            return Status::Error;
        }

        if (max_locations_distance_table > 0 &&
            (number_of_sources * number_of_destination >
             max_locations_distance_table * max_locations_distance_table))
        {
            json_result.values["status_message"] =
                "Number of entries " + std::to_string(number_of_sources * number_of_destination) +
                " is higher than current maximum (" +
                std::to_string(max_locations_distance_table * max_locations_distance_table) + ")";
            return Status::Error;
        }

        const bool checksum_OK = (route_parameters.check_sum == facade->GetCheckSum());

        std::vector<PhantomNodePair> phantom_node_source_vector(number_of_sources);
        std::vector<PhantomNodePair> phantom_node_target_vector(number_of_destination);
        auto phantom_node_source_out_iter = phantom_node_source_vector.begin();
        auto phantom_node_target_out_iter = phantom_node_target_vector.begin();
        for (const auto i : util::irange<std::size_t>(0u, route_parameters.coordinates.size()))
        {
            if (checksum_OK && i < route_parameters.hints.size() &&
                !route_parameters.hints[i].empty())
            {
                auto current_phantom_node = decodeBase64<PhantomNode>(route_parameters.hints[i]);
                if (current_phantom_node.IsValid(facade->GetNumberOfNodes()))
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
                if (!phantom_node_source_out_iter->first.IsValid(facade->GetNumberOfNodes()))
                {
                    json_result.values["status_message"] =
                        std::string("Could not find a matching segment for coordinate ") +
                        std::to_string(i);
                    return Status::NoSegment;
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
                if (!phantom_node_target_out_iter->first.IsValid(facade->GetNumberOfNodes()))
                {
                    json_result.values["status_message"] =
                        std::string("Could not find a matching segment for coordinate ") +
                        std::to_string(i);
                    return Status::NoSegment;
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
            json_result.values["status_message"] = "No distance table found";
            return Status::EmptyResult;
        }

        util::json::Array matrix_json_array;
        for (const auto row : util::irange<std::size_t>(0, number_of_sources))
        {
            util::json::Array json_row;
            auto row_begin_iterator = result_table->begin() + (row * number_of_destination);
            auto row_end_iterator = result_table->begin() + ((row + 1) * number_of_destination);
            json_row.values.insert(json_row.values.end(), row_begin_iterator, row_end_iterator);
            matrix_json_array.values.push_back(json_row);
        }
        json_result.values["distance_table"] = std::move(matrix_json_array);

        util::json::Array target_coord_json_array;
        for (const auto &phantom : snapped_target_phantoms)
        {
            util::json::Array json_coord;
            json_coord.values.push_back(phantom.location.lat / COORDINATE_PRECISION);
            json_coord.values.push_back(phantom.location.lon / COORDINATE_PRECISION);
            target_coord_json_array.values.push_back(json_coord);
        }
        json_result.values["destination_coordinates"] = std::move(target_coord_json_array);
        util::json::Array source_coord_json_array;
        for (const auto &phantom : snapped_source_phantoms)
        {
            util::json::Array json_coord;
            json_coord.values.push_back(phantom.location.lat / COORDINATE_PRECISION);
            json_coord.values.push_back(phantom.location.lon / COORDINATE_PRECISION);
            source_coord_json_array.values.push_back(json_coord);
        }
        json_result.values["source_coordinates"] = std::move(source_coord_json_array);
        return Status::Ok;
    }

  private:
    std::string descriptor_string;
    DataFacadeT *facade;
};
}
}
}

#endif // DISTANCE_TABLE_HPP
