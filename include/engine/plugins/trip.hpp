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
    mutable SearchEngineData heaps;
    mutable routing_algorithms::ShortestPathRouting shortest_path;
    mutable routing_algorithms::ManyToManyRouting duration_table;
    const int max_locations_trip;

    InternalRouteResult ComputeRoute(const std::shared_ptr<const datafacade::BaseDataFacade> facade,
                                     const std::vector<PhantomNode> &phantom_node_list,
                                     const std::vector<NodeID> &trip,
                                     const bool roundtrip) const;

  public:
    explicit TripPlugin(const int max_locations_trip_)
        : shortest_path(heaps), duration_table(heaps), max_locations_trip(max_locations_trip_)
    {
    }

    Status HandleRequest(const std::shared_ptr<const datafacade::BaseDataFacade> facade,
                         const api::TripParameters &parameters,
                         util::json::Object &json_result) const;
};
}
}
}

#endif // TRIP_HPP
