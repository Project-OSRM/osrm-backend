#include <osmium/osm/node.hpp>
#include <osmium/osm/node_ref.hpp>
#include <osmium/osm/types.hpp>
#include <osmium/osm/way.hpp>

#include <tbb/parallel_sort.h>

#include "extractor/node_locations_for_ways.hpp"

namespace osrm::extractor
{

NodeLocationsForWays::NodeLocationsForWays(std::vector<QueryNode> &all_nodes_list)
    : all_nodes_list{all_nodes_list}
{
}

void NodeLocationsForWays::prepare_for_lookup()
{
    // this is duplicated work, but the alternative would be to use an
    // osmium::handler::NodeLocationsForWays handler, which would read all nodes into
    // memory again
    tbb::parallel_sort(all_nodes_list.begin(),
                       all_nodes_list.end(),
                       [](const auto &left, const auto &right)
                       { return left.node_id < right.node_id; });
}

/**
 * Get location of node with given id.
 */
osmium::Location NodeLocationsForWays::get_node_location(OSMNodeID osm_id) const
{
    auto iter = std::lower_bound(all_nodes_list.begin(),
                                 all_nodes_list.end(),
                                 osm_id,
                                 [](auto it, auto val) { return it.node_id < val; });
    if (iter != all_nodes_list.end() && iter->node_id == osm_id)
    {
        return osmium::Location{from_alias<double>(util::toFloating(iter->lon)),
                                from_alias<double>(util::toFloating(iter->lat))};
    }
    return osmium::Location();
}

/**
 * Retrieve locations of all nodes in the way from the all_nodes_list and add them to
 * the way object.
 */
void NodeLocationsForWays::way(const osmium::Way &cway)
{
    // Technically we don't change the way, we only complete the nodes with their
    // locations. We need the parameter to be const because of the non-optimal
    // interface of the osmium library.
    osmium::Way &way = const_cast<osmium::Way &>(cway);
    for (auto &node_ref : way.nodes())
    {
        node_ref.set_location(get_node_location(OSMNodeID{node_ref.positive_ref()}));
        if (!node_ref.location().is_defined())
        {
            error = true;
        }
    }
}

} // namespace osrm::extractor
