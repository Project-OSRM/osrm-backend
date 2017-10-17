#ifndef ENGINE_API_TABLE_HPP
#define ENGINE_API_TABLE_HPP

#include "engine/api/base_api.hpp"
#include "engine/api/json_factory.hpp"
#include "engine/api/table_parameters.hpp"
#include "engine/api/waypoint.hpp"

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
    TableAPI(const datafacade::BaseDataFacade &facade_, const TableParameters &parameters_)
        : BaseAPI(facade_, parameters_), parameters(parameters_)
    {
    }

    virtual TableResult MakeResponse(const std::vector<EdgeWeight> &durations,
                                     const std::vector<PhantomNode> &phantoms) const
    {
        TableResult response;

        // symmetric case
        if (parameters.sources.empty())
        {
            response.sources = MakeWaypoints(phantoms);
        }
        else
        {
            response.sources = MakeWaypoints(phantoms, parameters.sources);
        }

        // symmetric case
        if (parameters.destinations.empty())
        {
            response.destinations = MakeWaypoints(phantoms);
        }
        else
        {
            response.destinations = MakeWaypoints(phantoms, parameters.destinations);
        }

        response.durations = std::move(durations);
        return response;
    }

  protected:
    virtual std::vector<Waypoint> MakeWaypoints(const std::vector<PhantomNode> &phantoms) const
    {
        std::vector<Waypoint> waypoints;
        waypoints.reserve(phantoms.size());
        BOOST_ASSERT(phantoms.size() == parameters.coordinates.size());

        boost::range::transform(
            phantoms, std::back_inserter(waypoints),
            [this](const PhantomNode &phantom) {
               auto name = facade.GetNameForID(facade.GetNameIndex(
                   phantom.forward_segment_id.id));
               return Waypoint{0, name.data(), phantom.location};
            });
        return waypoints;
    }

    virtual std::vector<Waypoint> MakeWaypoints(const std::vector<PhantomNode> &phantoms,
                                                const std::vector<std::size_t> &indices) const
    {
        std::vector<Waypoint> waypoints;
        waypoints.reserve(indices.size());
        boost::range::transform(
            indices, std::back_inserter(waypoints), [this, phantoms](const std::size_t idx) {
                BOOST_ASSERT(idx < phantoms.size());
                auto name = facade.GetNameForID(facade.GetNameIndex(phantoms[idx].forward_segment_id.id));
                return Waypoint{0, name.data(), phantoms[idx].location};
            });
        return waypoints;
    }

    const TableParameters &parameters;
};

} // ns api
} // ns engine
} // ns osrm

#endif
