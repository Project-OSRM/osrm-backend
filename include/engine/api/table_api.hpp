#ifndef ENGINE_API_TABLE_HPP
#define ENGINE_API_TABLE_HPP

#include "engine/api/base_api.hpp"
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
                 util::json::Object &response) const
    {
        auto number_of_sources = parameters.sources.size();
        auto number_of_destinations = parameters.destinations.size();

        // symmetric case
        if (parameters.sources.empty())
        {
            response.values["sources"] = MakeWaypoints(phantoms);
            number_of_sources = phantoms.size();
        }
        else
        {
            response.values["sources"] = MakeWaypoints(phantoms, parameters.sources);
        }

        if (parameters.destinations.empty())
        {
            response.values["destinations"] = MakeWaypoints(phantoms);
            number_of_destinations = phantoms.size();
        }
        else
        {
            response.values["destinations"] = MakeWaypoints(phantoms, parameters.destinations);
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

} // ns api
} // ns engine
} // ns osrm

#endif
