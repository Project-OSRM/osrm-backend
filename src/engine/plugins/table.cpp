#include "engine/plugins/table.hpp"

#include "engine/api/table_parameters.hpp"
#include "engine/object_encoder.hpp"
#include "engine/routing_algorithms/many_to_many.hpp"
#include "engine/search_engine_data.hpp"
#include "util/string_util.hpp"
#include "util/json_container.hpp"

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

TablePlugin::TablePlugin(datafacade::BaseDataFacade &facade, const int max_locations_distance_table)
    : BasePlugin{facade}, distance_table(&facade, heaps),
      max_locations_distance_table(max_locations_distance_table)
{
}

Status TablePlugin::HandleRequest(const api::TableParameters &params, util::json::Object &result)
{
    BOOST_ASSERT(params.IsValid());

    if (!CheckAllCoordinates(params.coordinates))
    {
        return Error("invalid-options", "Coordinates are invalid", result);
    }

    if (params.bearings.size() > 0 && params.coordinates.size() != params.bearings.size())
    {
        return Error("invalid-options", "Number of bearings does not match number of coordinates",
                     result);
    }

    if (max_locations_distance_table > 0 &&
        (params.sources.size() * params.destinations.size() >
         static_cast<std::size_t>(max_locations_distance_table * max_locations_distance_table)))
    {
        return Error(
            "invalid-options",
            "Number of entries " +
                std::to_string(params.sources.size() * params.destinations.size()) +
                " is higher than current maximum (" +
                std::to_string(max_locations_distance_table * max_locations_distance_table) + ")",
            result);
    }

    auto snapped_phantoms = SnapPhantomNodes(GetPhantomNodes(params));

    const auto result_table = [&]()
    {
        if (params.sources.empty())
        {
            BOOST_ASSERT(params.destinations.empty());
            return distance_table(snapped_phantoms);
        }
        else
        {
            return distance_table(snapped_phantoms, params.sources, params.destinations);
        }
    }();

    if (result_table.empty())
    {
        return Error("no-table", "No table found", result);
    }

    util::json::Array matrix_json_array;
    for (const auto row : util::irange<std::size_t>(0, params.sources.size()))
    {
        util::json::Array json_row;
        auto row_begin_iterator = result_table.begin() + (row * params.destinations.size());
        auto row_end_iterator = result_table.begin() + ((row + 1) * params.destinations.size());
        json_row.values.insert(json_row.values.end(), row_begin_iterator, row_end_iterator);
        matrix_json_array.values.push_back(std::move(json_row));
    }
    result.values["distance_table"] = matrix_json_array;

    // symmetric case
    if (params.sources.empty())
    {
        BOOST_ASSERT(params.destinations.empty());
        util::json::Array target_coord_json_array;
        for (const auto &phantom : snapped_phantoms)
        {
            util::json::Array json_coord;
            json_coord.values.push_back(phantom.location.lat / COORDINATE_PRECISION);
            json_coord.values.push_back(phantom.location.lon / COORDINATE_PRECISION);
            target_coord_json_array.values.push_back(std::move(json_coord));
        }
        result.values["destination_coordinates"] = std::move(target_coord_json_array);
        util::json::Array source_coord_json_array;
        for (const auto &phantom : snapped_phantoms)
        {
            util::json::Array json_coord;
            json_coord.values.push_back(phantom.location.lat / COORDINATE_PRECISION);
            json_coord.values.push_back(phantom.location.lon / COORDINATE_PRECISION);
            source_coord_json_array.values.push_back(std::move(json_coord));
        }
        result.values["source_coordinates"] = std::move(source_coord_json_array);
    }
    // asymmetric case
    else
    {
        BOOST_ASSERT(!params.destinations.empty());

        util::json::Array target_coord_json_array;
        for (const auto index : params.sources)
        {
            const auto &phantom = snapped_phantoms[index];
            util::json::Array json_coord;
            json_coord.values.push_back(phantom.location.lat / COORDINATE_PRECISION);
            json_coord.values.push_back(phantom.location.lon / COORDINATE_PRECISION);
            target_coord_json_array.values.push_back(std::move(json_coord));
        }
        result.values["destination_coordinates"] = std::move(target_coord_json_array);
        util::json::Array source_coord_json_array;
        for (const auto index : params.sources)
        {
            const auto &phantom = snapped_phantoms[index];
            util::json::Array json_coord;
            json_coord.values.push_back(phantom.location.lat / COORDINATE_PRECISION);
            json_coord.values.push_back(phantom.location.lon / COORDINATE_PRECISION);
            source_coord_json_array.values.push_back(std::move(json_coord));
        }
        result.values["source_coordinates"] = std::move(source_coord_json_array);
    }
    return Status::Ok;
}
}
}
}
