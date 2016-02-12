#ifndef DISTANCE_TABLE_HPP
#define DISTANCE_TABLE_HPP

#include "engine/plugins/plugin_base.hpp"

#include "engine/api/table_parameters.hpp"
#include "engine/object_encoder.hpp"
#include "engine/routing_algorithms/many_to_many.hpp"
#include "engine/search_engine_data.hpp"
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

class DistanceTablePlugin final : public BasePlugin
{
  private:
    SearchEngineData heaps;
    routing_algorithms::ManyToManyRouting<datafacade::BaseDataFacade> distance_table;
    int max_locations_distance_table;

  public:
    explicit DistanceTablePlugin(datafacade::BaseDataFacade *facade,
                                 const int max_locations_distance_table)
        : BasePlugin{*facade}, distance_table(facade, heaps),
          max_locations_distance_table(max_locations_distance_table)
    {
    }

    Status HandleRequest(const api::TableParameters &params, util::json::Object &result)
    {
        BOOST_ASSERT(params.IsValid());

        if (!CheckAllCoordinates(params.coordinates))
            return Error("invalid-options", "Coordinates are invalid", result);

        if (params.bearings.size() > 0 && params.coordinates.size() != params.bearings.size())
          return Error("invalid-options", "Number of bearings does not match number of coordinates", result);

        if (max_locations_distance_table > 0 &&
            (params.sources.size() * params.destinations.size() >
             max_locations_distance_table * max_locations_distance_table))
          return Error("invalid-options", "Number of entries " + std::to_string(params.sources.size() * params.destinations.size()) +
                " is higher than current maximum (" +
                std::to_string(max_locations_distance_table * max_locations_distance_table) + ")", result);

        const bool checksum_OK = (params.check_sum == BasePlugin::facade.GetCheckSum());

        std::vector<PhantomNodePair> phantom_node_source_vector(params.sources.size());
        std::vector<PhantomNodePair> phantom_node_target_vector(params.destinations.size());
        auto phantom_node_source_out_iter = phantom_node_source_vector.begin();
        auto phantom_node_target_out_iter = phantom_node_target_vector.begin();
        for (const auto i : util::irange<std::size_t>(0u, params.coordinates.size()))
        {
            if (checksum_OK && i < params.hints.size() && !params.hints[i].empty())
            {
                PhantomNode current_phantom_node;
                ObjectEncoder::DecodeFromBase64(params.hints[i], current_phantom_node);
                if (current_phantom_node.IsValid(BasePlugin::facade.GetNumberOfNodes()))
                {
                    if (params.is_source[i])
                    {
                        *phantom_node_source_out_iter =
                            std::make_pair(current_phantom_node, current_phantom_node);
                        if (params.is_destination[i])
                        {
                            *phantom_node_target_out_iter = *phantom_node_source_out_iter;
                            phantom_node_target_out_iter++;
                        }
                        phantom_node_source_out_iter++;
                    }
                    else
                    {
                        BOOST_ASSERT(params.is_destination[i] && !params.is_source[i]);
                        *phantom_node_target_out_iter =
                            std::make_pair(current_phantom_node, current_phantom_node);
                        phantom_node_target_out_iter++;
                    }
                    continue;
                }
            }
            const int bearing = params.bearings.size() > 0 ? params.bearings[i].first : 0;
            const int range = params.bearings.size() > 0
                                  ? (params.bearings[i].second ? *param_bearings[i].second : 10)
                                  : 180;
            if (params.is_source[i])
            {
                *phantom_node_source_out_iter =
                    BasePlugin::facade.NearestPhantomNodeWithAlternativeFromBigComponent(
                        params.coordinates[i], bearing, range);
                // we didn't found a fitting node, return error
                if (!phantom_node_source_out_iter->first.IsValid(
                        BasePlugin::facade.GetNumberOfNodes()))
                {
                    result.values["status_message"] =
                        std::string("Could not find a matching segment for coordinate ") +
                        std::to_string(i);
                    return Status::NoSegment;
                }

                if (params.is_destination[i])
                {
                    *phantom_node_target_out_iter = *phantom_node_source_out_iter;
                    phantom_node_target_out_iter++;
                }
                phantom_node_source_out_iter++;
            }
            else
            {
                BOOST_ASSERT(params.is_destination[i] && !params.is_source[i]);

                *phantom_node_target_out_iter =
                    BasePlugin::facade.NearestPhantomNodeWithAlternativeFromBigComponent(
                        params.coordinates[i], bearing, range);
                // we didn't found a fitting node, return error
                if (!phantom_node_target_out_iter->first.IsValid(
                        BasePlugin::facade.GetNumberOfNodes()))
                {
                    result.values["status_message"] =
                        std::string("Could not find a matching segment for coordinate ") +
                        std::to_string(i);
                    return Status::NoSegment;
                }
                phantom_node_target_out_iter++;
            }
        }

        BOOST_ASSERT((phantom_node_source_out_iter - phantom_node_source_vector.begin()) ==
                     params.sources.size());
        BOOST_ASSERT((phantom_node_target_out_iter - phantom_node_target_vector.begin()) ==
                     params.destinations.size());

        // FIXME we should clear phantom_node_source_vector and phantom_node_target_vector after
        // this
        auto snapped_source_phantoms = snapPhantomNodes(phantom_node_source_vector);
        auto snapped_target_phantoms = snapPhantomNodes(phantom_node_target_vector);

        auto result_table = distance_table(snapped_source_phantoms, snapped_target_phantoms);

        if (!result_table)
        {
            result.values["status_message"] = "No distance table found";
            return Status::EmptyResult;
        }

        util::json::Array matrix_json_array;
        for (const auto row : util::irange<std::size_t>(0, params.sources.size()))
        {
            util::json::Array json_row;
            auto row_begin_iterator = result_table->begin() + (row * params.destinations.size());
            auto row_end_iterator =
                result_table->begin() + ((row + 1) * params.destinations.size());
            json_row.values.insert(json_row.values.end(), row_begin_iterator, row_end_iterator);
            matrix_json_array.values.push_back(json_row);
        }
        result.values["distance_table"] = matrix_json_array;

        util::json::Array target_coord_json_array;
        for (const auto &phantom : snapped_target_phantoms)
        {
            util::json::Array json_coord;
            json_coord.values.push_back(phantom.location.lat / COORDINATE_PRECISION);
            json_coord.values.push_back(phantom.location.lon / COORDINATE_PRECISION);
            target_coord_json_array.values.push_back(json_coord);
        }
        result.values["destination_coordinates"] = target_coord_json_array;
        util::json::Array source_coord_json_array;
        for (const auto &phantom : snapped_source_phantoms)
        {
            util::json::Array json_coord;
            json_coord.values.push_back(phantom.location.lat / COORDINATE_PRECISION);
            json_coord.values.push_back(phantom.location.lon / COORDINATE_PRECISION);
            source_coord_json_array.values.push_back(json_coord);
        }
        result.values["source_coordinates"] = source_coord_json_array;
        return Status::Ok;
    }
};
}
}
}

#endif // DISTANCE_TABLE_HPP
