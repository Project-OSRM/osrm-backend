#ifndef TRIP_HPP
#define TRIP_HPP

#include "engine/plugins/plugin_base.hpp"

#include "engine/api/trip_parameters.hpp"
#include "engine/routing_algorithms/many_to_many.hpp"
#include "engine/routing_algorithms/shortest_path.hpp"

#include "osrm/json_container.hpp"

#include <boost/assert.hpp>

#include <algorithm>
#include <cstdlib>
#include <iterator>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace osrm
{
namespace engine
{
namespace plugins
{

class TripPlugin final : public BasePlugin
{
  private:
    SearchEngineData heaps;
    routing_algorithms::ShortestPathRouting<datafacade::BaseDataFacade> shortest_path;
    routing_algorithms::ManyToManyRouting<datafacade::BaseDataFacade> duration_table;
    int max_locations_trip;

    InternalRouteResult ComputeRoute(const std::vector<PhantomNode> &phantom_node_list,
                                     const std::vector<NodeID> &trip);

  public:
    explicit TripPlugin(datafacade::BaseDataFacade &facade_,
                        const int max_locations_trip_,
                        const double max_radius_when_bearings_)
        : BasePlugin(facade_, max_radius_when_bearings_), shortest_path(&facade_, heaps),
          duration_table(&facade_, heaps), max_locations_trip(max_locations_trip_)
    {
    }

    Status HandleRequest(const api::TripParameters &parameters, util::json::Object &json_result);
};
}
}
}

#endif // TRIP_HPP
