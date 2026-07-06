#ifndef TREE_HPP
#define TREE_HPP

#include "engine/api/tree_parameters.hpp"
#include "engine/datafacade/contiguous_internalmem_datafacade.hpp"
#include "engine/plugins/plugin_base.hpp"
#include "engine/routing_algorithms.hpp"
#include "osrm/json_container.hpp"

namespace osrm::engine::plugins
{

// Highway Mode 2 "charger-ahead" plugin.
//
// Stage 3 (recursive tree): snaps the input coordinate with the bearing filter, verifies the snap
// is on a motorway, then walks the mainline continuation forward through the edge-based graph up
// to hard_cap_m, and recurses into every qualifying motorway-to-motorway junction (OffRamp/Fork
// whose destination ref matches a motorway pattern), following the ramp connector until motorway
// class resumes. Emits the Contract 1 nested tree schema (branches[] of {junction, route}); with
// debug=true each segment also carries a junctions[] debug array. MLD only (needs the edge-based
// graph adjacency the MLD facade exposes).
class TreePlugin final : public BasePlugin
{
  public:
    explicit TreePlugin(const std::optional<double> default_radius);

    Status HandleRequest(const RoutingAlgorithmsInterface &algorithms,
                         const api::TreeParameters &params,
                         osrm::engine::api::ResultT &result) const;
};
} // namespace osrm::engine::plugins

#endif /* TREE_HPP */
