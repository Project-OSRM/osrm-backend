#ifndef ENGINE_API_TABLE_HPP
#define ENGINE_API_TABLE_HPP

#include "engine/api/base_api.hpp"
#include "engine/api/table_parameters.hpp"
#include "engine/api/json_factory.hpp"

#include "engine/datafacade/datafacade_base.hpp"

#include "engine/guidance/assemble_leg.hpp"
#include "engine/guidance/assemble_route.hpp"
#include "engine/guidance/assemble_geometry.hpp"
#include "engine/guidance/assemble_overview.hpp"
#include "engine/guidance/assemble_steps.hpp"

#include "engine/internal_route_result.hpp"

#include "util/integer_range.hpp"

namespace osrm
{
namespace engine
{
namespace api
{

class TableAPI final : public BaseAPI
{
  public:
    TableAPI(const datafacade::BaseDataFacade &facade_, const TableParameters &parameters_)
        : BaseAPI(facade_, parameters_), parameters(parameters_)
    {
    }

    virtual void MakeResponse(const std::vector<EdgeWeight> &durations,
                              const std::vector<PhantomNode> &phantoms,
                              util::json::Object &response) const
    {
        // symmetric case
        if (parameters.sources.empty())
        {
            BOOST_ASSERT(parameters.destinations.empty());
            response.values["sources"] = MakeWaypoints(phantoms);
            response.values["destinations"] = MakeWaypoints(phantoms);
            response.values["durations"] = MakeTable(durations, phantoms.size(), phantoms.size());
        }
        else
        {
            response.values["sources"] = MakeWaypoints(phantoms, parameters.sources);
            response.values["destinations"] = MakeWaypoints(phantoms, parameters.destinations);
            response.values["durations"] =
                MakeTable(durations, parameters.sources.size(), parameters.destinations.size());
        }
        response.values["code"] = "ok";
    }

  protected:
    virtual util::json::Array MakeWaypoints(const std::vector<PhantomNode> &phantoms) const
    {
        util::json::Array json_waypoints;
        json_waypoints.values.reserve(phantoms.size());
        BOOST_ASSERT(phantoms.size() == parameters.coordinates.size());
        auto phantom_iter = phantoms.begin();
        auto coordinate_iter = parameters.coordinates.begin();
        for (; phantom_iter != phantoms.end() && coordinate_iter != parameters.coordinates.end();
             ++phantom_iter, ++coordinate_iter)
        {
            json_waypoints.values.push_back(BaseAPI::MakeWaypoint(*coordinate_iter, *phantom_iter));
        }
        return json_waypoints;
    }

    virtual util::json::Array MakeWaypoints(const std::vector<PhantomNode> &phantoms,
                                            const std::vector<std::size_t> &indices) const
    {
        util::json::Array json_waypoints;
        json_waypoints.values.reserve(indices.size());
        for (auto idx : indices)
        {
            BOOST_ASSERT(idx < phantoms.size() && idx < parameters.coordinates.size());
            json_waypoints.values.push_back(
                BaseAPI::MakeWaypoint(parameters.coordinates[idx], phantoms[idx]));
        }
        return json_waypoints;
    }

    virtual util::json::Array MakeTable(const std::vector<EdgeWeight> &values,
                                        std::size_t number_of_rows,
                                        std::size_t number_of_columns) const
    {
        util::json::Array json_table;
        for (const auto row : util::irange<std::size_t>(0, number_of_rows))
        {
            util::json::Array json_row;
            auto row_begin_iterator = values.begin() + (row * number_of_columns);
            auto row_end_iterator = values.begin() + ((row + 1) * number_of_columns);
            json_row.values.insert(json_row.values.end(), row_begin_iterator, row_end_iterator);
            json_table.values.push_back(std::move(json_row));
        }
        return json_table;
    }

    const TableParameters &parameters;
};

} // ns api
} // ns engine
} // ns osrm

#endif
