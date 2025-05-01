#ifndef OSRM_EXTRACTOR_AREA_DIJKSTRA_HPP
#define OSRM_EXTRACTOR_AREA_DIJKSTRA_HPP

#include "extractor/area/typedefs.hpp"

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <boost/graph/dijkstra_shortest_paths_no_color_map.hpp>
#include <boost/graph/graph_traits.hpp>

#include <iterator>
#include <osmium/osm.hpp>
#include <osmium/osm/node_ref.hpp>
#include <osmium/osm/types.hpp>

namespace osrm::extractor::area
{

/**
 * Runs the Dijkstra shortest path algorithm on the area polygon and the visibility
 * graph.
 *
 * We do not want to keep all visible edges we found, only those that are part of a
 * shortest path between any two entry points to the area. We use Dijkstra to find all
 * these shortest paths.
 */
class Dijkstra
{
    using vertices_t = std::vector<osmium::NodeRef>;
    using vertices_iter_t = vertices_t::iterator;
    using graph_t = boost::adjacency_list<boost::vecS,
                                          boost::vecS,
                                          boost::undirectedS,
                                          boost::no_property,
                                          boost::property<boost::edge_weight_t, double>>;
    using vertex_descriptor = boost::graph_traits<graph_t>::vertex_descriptor;
    using Edge = std::pair<vertex_descriptor, vertex_descriptor>;

  public:
    Dijkstra(const OsmiumPolygon &poly, std::set<OsmiumSegment> &vis_map);
    std::set<OsmiumSegment> run(const NodeRefSet &entry_points);

  private:
    vertices_iter_t find(const osmium::NodeRef &n)
    {
        return std::lower_bound(vertices.begin(),
                                vertices.end(),
                                n,
                                [](const osmium::NodeRef &u, const osmium::NodeRef &v)
                                { return u < v; });
    }
    vertex_descriptor indexOf(const osmium::NodeRef &n)
    {
        return std::distance(vertices.begin(), find(n));
    }
    vertex_descriptor insert(const osmium::NodeRef &n)
    {
        vertices_iter_t found = find(n);
        if (found != vertices.end() && *found == n)
            return std::distance(vertices.begin(), found);
        return std::distance(vertices.begin(), vertices.insert(found, n));
    };

    /** The vertices, unique and sorted. */
    vertices_t vertices;
    /**
     * Temporary store for segments. We must have inserted all vertices before starting
     * on edges because edges are defined as pair of indices into the (sorted) vertices
     * vector.
     */
    std::set<OsmiumSegment> poly_segments;
    std::set<OsmiumSegment> &vis_map;
};

} // namespace osrm::extractor::area

#endif
