#ifndef ENGINE_API_TABLE_HPP
#define ENGINE_API_TABLE_HPP

#include "engine/api/base_api.hpp"
#include "engine/api/base_result.hpp"
#include "engine/api/json_factory.hpp"
#include "engine/api/table_parameters.hpp"

#include "engine/datafacade/datafacade_base.hpp"

#include "engine/guidance/assemble_geometry.hpp"
#include "engine/guidance/assemble_leg.hpp"
#include "engine/guidance/assemble_overview.hpp"
#include "engine/guidance/assemble_route.hpp"
#include "engine/guidance/assemble_steps.hpp"

#include "engine/internal_route_result.hpp"

#include "util/integer_range.hpp"

#include <boost/range/algorithm/transform.hpp>

#include <iterator>

namespace osrm
{
namespace engine
{
namespace api
{

class TableAPI final : public BaseAPI
{
  public:
    struct TableCellRef
    {
        TableCellRef(const std::size_t &row, const std::size_t &column) : row{row}, column{column}
        {
        }
        std::size_t row;
        std::size_t column;
    };

    TableAPI(const datafacade::BaseDataFacade &facade_, const TableParameters &parameters_)
        : BaseAPI(facade_, parameters_), parameters(parameters_)
    {
    }

    virtual void
    MakeResponse(const std::pair<std::vector<EdgeDuration>, std::vector<EdgeDistance>> &tables,
                 const std::vector<PhantomNode> &phantoms,
                 const std::vector<TableCellRef> &fallback_speed_cells,
                 osrm::engine::api::ResultT &response) const
    {
        if (response.is<flatbuffers::FlatBufferBuilder>())
        {
            auto &fb_result = response.get<flatbuffers::FlatBufferBuilder>();
            MakeResponse(tables, phantoms, fallback_speed_cells, fb_result);
        }
        else
        {
            auto &json_result = response.get<util::json::Object>();
            MakeResponse(tables, phantoms, fallback_speed_cells, json_result);
        }
    }

    virtual void
    MakeResponse(const std::pair<std::vector<EdgeDuration>, std::vector<EdgeDistance>> &tables,
                 const std::vector<PhantomNode> &phantoms,
                 const std::vector<TableCellRef> &fallback_speed_cells,
                 flatbuffers::FlatBufferBuilder &fb_result) const
    {
        auto number_of_sources = parameters.sources.size();
        auto number_of_destinations = parameters.destinations.size();

        auto data_timestamp = facade.GetTimestamp();
        flatbuffers::Offset<flatbuffers::String> data_version_string;
        if (!data_timestamp.empty())
        {
            data_version_string = fb_result.CreateString(data_timestamp);
        }

        // symmetric case
        flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<fbresult::Waypoint>>> sources;
        if (parameters.sources.empty())
        {
            if (!parameters.skip_waypoints)
            {
                sources = MakeWaypoints(fb_result, phantoms);
            }
            number_of_sources = phantoms.size();
        }
        else
        {
            if (!parameters.skip_waypoints)
            {
                sources = MakeWaypoints(fb_result, phantoms, parameters.sources);
            }
        }

        flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<fbresult::Waypoint>>>
            destinations;
        if (parameters.destinations.empty())
        {
            if (!parameters.skip_waypoints)
            {
                destinations = MakeWaypoints(fb_result, phantoms);
            }
            number_of_destinations = phantoms.size();
        }
        else
        {
            if (!parameters.skip_waypoints)
            {
                destinations = MakeWaypoints(fb_result, phantoms, parameters.destinations);
            }
        }

        bool use_durations = parameters.annotations & TableParameters::AnnotationsType::Duration;
        flatbuffers::Offset<flatbuffers::Vector<float>> durations;
        if (use_durations)
        {
            durations = MakeDurationTable(fb_result, tables.first);
        }

        bool use_distances = parameters.annotations & TableParameters::AnnotationsType::Distance;
        flatbuffers::Offset<flatbuffers::Vector<float>> distances;
        if (use_distances)
        {
            distances = MakeDistanceTable(fb_result, tables.second);
        }

        bool have_speed_cells =
            parameters.fallback_speed != INVALID_FALLBACK_SPEED && parameters.fallback_speed > 0;
        flatbuffers::Offset<flatbuffers::Vector<uint32_t>> speed_cells;
        if (have_speed_cells)
        {
            speed_cells = MakeEstimatesTable(fb_result, fallback_speed_cells);
        }

        fbresult::TableBuilder table(fb_result);
        table.add_destinations(destinations);
        table.add_rows(number_of_sources);
        table.add_cols(number_of_destinations);
        if (use_durations)
        {
            table.add_durations(durations);
        }
        if (use_distances)
        {
            table.add_distances(distances);
        }
        if (have_speed_cells)
        {
            table.add_fallback_speed_cells(speed_cells);
        }
        auto table_buffer = table.Finish();

        fbresult::FBResultBuilder response(fb_result);
        if (!data_timestamp.empty())
        {
            response.add_data_version(data_version_string);
        }
        response.add_table(table_buffer);
        response.add_waypoints(sources);
        fb_result.Finish(response.Finish());
    }

    virtual void
    MakeResponse(const std::pair<std::vector<EdgeDuration>, std::vector<EdgeDistance>> &tables,
                 const std::vector<PhantomNode> &phantoms,
                 const std::vector<TableCellRef> &fallback_speed_cells,
                 util::json::Object &response) const
    {
        auto number_of_sources = parameters.sources.size();
        auto number_of_destinations = parameters.destinations.size();

        // symmetric case
        if (parameters.sources.empty())
        {
            if (!parameters.skip_waypoints)
            {
                response.values["sources"] = MakeWaypoints(phantoms);
            }
            number_of_sources = phantoms.size();
        }
        else
        {
            if (!parameters.skip_waypoints)
            {
                response.values["sources"] = MakeWaypoints(phantoms, parameters.sources);
            }
        }

        if (parameters.destinations.empty())
        {
            if (!parameters.skip_waypoints)
            {
                response.values["destinations"] = MakeWaypoints(phantoms);
            }
            number_of_destinations = phantoms.size();
        }
        else
        {
            if (!parameters.skip_waypoints)
            {
                response.values["destinations"] = MakeWaypoints(phantoms, parameters.destinations);
            }
        }

        if (parameters.annotations & TableParameters::AnnotationsType::Duration)
        {
            response.values["durations"] =
                MakeDurationTable(tables.first, number_of_sources, number_of_destinations);
        }

        if (parameters.annotations & TableParameters::AnnotationsType::Distance)
        {
            response.values["distances"] =
                MakeDistanceTable(tables.second, number_of_sources, number_of_destinations);
        }

        if (parameters.fallback_speed != INVALID_FALLBACK_SPEED && parameters.fallback_speed > 0)
        {
            response.values["fallback_speed_cells"] = MakeEstimatesTable(fallback_speed_cells);
        }

        response.values["code"] = "Ok";
    }

  protected:
    virtual flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<fbresult::Waypoint>>>
    MakeWaypoints(flatbuffers::FlatBufferBuilder &builder,
                  const std::vector<PhantomNode> &phantoms) const
    {
        std::vector<flatbuffers::Offset<fbresult::Waypoint>> waypoints;
        waypoints.reserve(phantoms.size());
        BOOST_ASSERT(phantoms.size() == parameters.coordinates.size());

        boost::range::transform(
            phantoms, std::back_inserter(waypoints), [this, &builder](const PhantomNode &phantom) {
                return BaseAPI::MakeWaypoint(&builder, phantom)->Finish();
            });
        return builder.CreateVector(waypoints);
    }

    virtual flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<fbresult::Waypoint>>>
    MakeWaypoints(flatbuffers::FlatBufferBuilder &builder,
                  const std::vector<PhantomNode> &phantoms,
                  const std::vector<std::size_t> &indices) const
    {
        std::vector<flatbuffers::Offset<fbresult::Waypoint>> waypoints;
        waypoints.reserve(indices.size());
        boost::range::transform(indices,
                                std::back_inserter(waypoints),
                                [this, &builder, phantoms](const std::size_t idx) {
                                    BOOST_ASSERT(idx < phantoms.size());
                                    return BaseAPI::MakeWaypoint(&builder, phantoms[idx])->Finish();
                                });
        return builder.CreateVector(waypoints);
    }

    virtual flatbuffers::Offset<flatbuffers::Vector<float>>
    MakeDurationTable(flatbuffers::FlatBufferBuilder &builder,
                      const std::vector<EdgeWeight> &values) const
    {
        std::vector<float> distance_table;
        distance_table.resize(values.size());
        std::transform(
            values.begin(), values.end(), distance_table.begin(), [](const EdgeWeight duration) {
                if (duration == MAXIMAL_EDGE_DURATION)
                {
                    return 0.;
                }
                return duration / 10.;
            });
        return builder.CreateVector(distance_table);
    }

    virtual flatbuffers::Offset<flatbuffers::Vector<float>>
    MakeDistanceTable(flatbuffers::FlatBufferBuilder &builder,
                      const std::vector<EdgeDistance> &values) const
    {
        std::vector<float> duration_table;
        duration_table.resize(values.size());
        std::transform(
            values.begin(), values.end(), duration_table.begin(), [](const EdgeDistance distance) {
                if (distance == INVALID_EDGE_DISTANCE)
                {
                    return 0.;
                }
                return std::round(distance * 10) / 10.;
            });
        return builder.CreateVector(duration_table);
    }

    virtual flatbuffers::Offset<flatbuffers::Vector<uint32_t>>
    MakeEstimatesTable(flatbuffers::FlatBufferBuilder &builder,
                       const std::vector<TableCellRef> &fallback_speed_cells) const
    {
        std::vector<uint32_t> fb_table;
        fb_table.reserve(fallback_speed_cells.size());
        std::for_each(
            fallback_speed_cells.begin(), fallback_speed_cells.end(), [&](const auto &cell) {
                fb_table.push_back(cell.row);
                fb_table.push_back(cell.column);
            });
        return builder.CreateVector(fb_table);
    }

    virtual util::json::Array MakeWaypoints(const std::vector<PhantomNode> &phantoms) const
    {
        util::json::Array json_waypoints;
        json_waypoints.values.reserve(phantoms.size());
        BOOST_ASSERT(phantoms.size() == parameters.coordinates.size());

        boost::range::transform(
            phantoms,
            std::back_inserter(json_waypoints.values),
            [this](const PhantomNode &phantom) { return BaseAPI::MakeWaypoint(phantom); });
        return json_waypoints;
    }

    virtual util::json::Array MakeWaypoints(const std::vector<PhantomNode> &phantoms,
                                            const std::vector<std::size_t> &indices) const
    {
        util::json::Array json_waypoints;
        json_waypoints.values.reserve(indices.size());
        boost::range::transform(indices,
                                std::back_inserter(json_waypoints.values),
                                [this, phantoms](const std::size_t idx) {
                                    BOOST_ASSERT(idx < phantoms.size());
                                    return BaseAPI::MakeWaypoint(phantoms[idx]);
                                });
        return json_waypoints;
    }

    virtual util::json::Array MakeDurationTable(const std::vector<EdgeWeight> &values,
                                                std::size_t number_of_rows,
                                                std::size_t number_of_columns) const
    {
        util::json::Array json_table;
        for (const auto row : util::irange<std::size_t>(0UL, number_of_rows))
        {
            util::json::Array json_row;
            auto row_begin_iterator = values.begin() + (row * number_of_columns);
            auto row_end_iterator = values.begin() + ((row + 1) * number_of_columns);
            json_row.values.resize(number_of_columns);
            std::transform(row_begin_iterator,
                           row_end_iterator,
                           json_row.values.begin(),
                           [](const EdgeWeight duration) {
                               if (duration == MAXIMAL_EDGE_DURATION)
                               {
                                   return util::json::Value(util::json::Null());
                               }
                               // division by 10 because the duration is in deciseconds (10s)
                               return util::json::Value(util::json::Number(duration / 10.));
                           });
            json_table.values.push_back(std::move(json_row));
        }
        return json_table;
    }

    virtual util::json::Array MakeDistanceTable(const std::vector<EdgeDistance> &values,
                                                std::size_t number_of_rows,
                                                std::size_t number_of_columns) const
    {
        util::json::Array json_table;
        for (const auto row : util::irange<std::size_t>(0UL, number_of_rows))
        {
            util::json::Array json_row;
            auto row_begin_iterator = values.begin() + (row * number_of_columns);
            auto row_end_iterator = values.begin() + ((row + 1) * number_of_columns);
            json_row.values.resize(number_of_columns);
            std::transform(row_begin_iterator,
                           row_end_iterator,
                           json_row.values.begin(),
                           [](const EdgeDistance distance) {
                               if (distance == INVALID_EDGE_DISTANCE)
                               {
                                   return util::json::Value(util::json::Null());
                               }
                               // round to single decimal place
                               return util::json::Value(
                                   util::json::Number(std::round(distance * 10) / 10.));
                           });
            json_table.values.push_back(std::move(json_row));
        }
        return json_table;
    }

    virtual util::json::Array
    MakeEstimatesTable(const std::vector<TableCellRef> &fallback_speed_cells) const
    {
        util::json::Array json_table;
        std::for_each(
            fallback_speed_cells.begin(), fallback_speed_cells.end(), [&](const auto &cell) {
                util::json::Array row;
                row.values.push_back(util::json::Number(cell.row));
                row.values.push_back(util::json::Number(cell.column));
                json_table.values.push_back(std::move(row));
            });
        return json_table;
    }

    const TableParameters &parameters;
};

} // namespace api
} // namespace engine
} // namespace osrm

#endif
