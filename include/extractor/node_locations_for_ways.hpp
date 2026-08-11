#ifndef NODE_LOCATIONS_FOR_WAYS_HPP
#define NODE_LOCATIONS_FOR_WAYS_HPP

#include "extractor/query_node.hpp"

#include <osmium/handler.hpp>
#include <osmium/osm/location.hpp>

namespace osrm::extractor
{

/**
 * Handler to add node locations to ways.
 *
 * Compatible to osmium::handler::NodeLocationsForWays but uses the node locations we
 * already have in memory.
 */
class NodeLocationsForWays : public osmium::handler::Handler
{
    std::vector<QueryNode> &all_nodes_list;
    osmium::Location get_node_location(OSMNodeID osm_id) const;

  public:
    NodeLocationsForWays(std::vector<QueryNode> &all_nodes_list);
    void way(const osmium::Way &way);
    void prepare_for_lookup();
    bool error{false};
};

} // namespace osrm::extractor

#endif // NODE_LOCATIONS_FOR_WAYS_HPP
