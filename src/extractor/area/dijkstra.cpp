#include "extractor/area/dijkstra.hpp"

#include "extractor/area/typedefs.hpp"
#include "extractor/area/util.hpp"
#include "util/log.hpp"

#include <algorithm>
#include <boost/geometry/algorithms/detail/distance/interface.hpp>
#include <boost/geometry/algorithms/distance.hpp>
#include <osmium/osm/node_ref.hpp>

#include <cmath>
#include <set>
#include <vector>

namespace osrm::extractor::area
{

/**
 * Penalty added to generated segments. Generated segments are kept only if they are
 * somewhat shorter than following the area perimeter.
 */
const double PENALTY = 1.1;

Dijkstra::Dijkstra(const OsmiumPolygon &poly, std::set<OsmiumSegment> &vis_map) : vis_map{vis_map}
{
    for_each_ring(poly,
                  [&](auto &ring)
                  {
                      for_each_pair_in_ring(ring,
                                            [&](const osmium::NodeRef &u, const osmium::NodeRef &v)
                                            { poly_segments.emplace(OsmiumSegment(u, v)); });
                  });
};

/**
 * Run the Dijkstra shortest-path algorithm on the visibility graph.
 */
std::set<OsmiumSegment> Dijkstra::run(const NodeRefSet &entry_points)
{
    using namespace boost;
    using Edge = std::pair<vertex_descriptor, vertex_descriptor>;
    std::vector<Edge> edges;
    std::vector<double> weights;

    // The visibility graph vis_map may also contain segments of the polygon, but we
    // want to treat them different. Cleanly separate the generated segments from those
    // in the map.
    std::set<OsmiumSegment> all_segments;
    all_segments.insert(poly_segments.begin(), poly_segments.end());
    all_segments.insert(vis_map.begin(), vis_map.end());

    std::set<OsmiumSegment> generated_segments;
    std::set_difference(all_segments.begin(),
                        all_segments.end(),
                        poly_segments.begin(),
                        poly_segments.end(),
                        std::inserter(generated_segments, generated_segments.end()));

    util::Log(logDEBUG) << "Running Dijkstra on: " << entry_points.size() << " entry points";

    assert(poly_segments.size() + generated_segments.size() == all_segments.size());

    // Do NOT combine this loop with the others! All vertices must be inserted before
    // starting on edges.
    for (const OsmiumSegment &s : all_segments)
    {
        insert(s.first);
        insert(s.second);
    }

    util::Log(logDEBUG) << "  The polygon has " << poly_segments.size() << " edges:";
    for (const OsmiumSegment &s : poly_segments)
    {
        edges.emplace_back(indexOf(s.first), indexOf(s.second));
        double weight = bg::distance(s.first, s.second);
        weights.push_back(weight);
        util::Log(logDEBUG) << "    " << s.first.ref() << " -> " << s.second.ref() << " " << weight;
    }
    util::Log(logDEBUG) << "  And the following " << generated_segments.size()
                        << " edges were generated: ";
    for (const OsmiumSegment &s : generated_segments)
    {
        edges.emplace_back(indexOf(s.first), indexOf(s.second));
        double weight = bg::distance(s.first, s.second) * PENALTY;
        weights.push_back(weight);
        util::Log(logDEBUG) << "    " << s.first.ref() << " -> " << s.second.ref() << " " << weight;
    }
    assert(edges.size() == all_segments.size());

    util::Log(logDEBUG) << "Init Dijkstra with " << edges.size() << " edges and " << vertices.size()
                        << " vertices";
    graph_t g(edges.begin(), edges.end(), weights.begin(), vertices.size());

    util::Log(logDEBUG) << "Running Dijkstra on: " << entry_points.size() << " entry points, "
                        << num_vertices(g) << " vertices and " << num_edges(g) << " edges.";

    std::set<OsmiumSegment> result;

    for (const osmium::NodeRef &entry_point : entry_points)
    {
        vertex_descriptor u = indexOf(entry_point);
        std::vector<vertex_descriptor> p(vertices.size());
        dijkstra_shortest_paths(g,
                                u,
                                predecessor_map(boost::make_iterator_property_map(
                                    p.begin(), get(boost::vertex_index, g))));

        // starting from each exit point report all generated edges that are on the
        // shortest path from the entry point

        for (const osmium::NodeRef &exit_point : entry_points)
        {
            util::Log(logDEBUG) << "  Collecting segments from " << entry_point.ref() << " -> "
                                << exit_point.ref();
            vertex_descriptor v = indexOf(exit_point);
            while (v != u && v != p[v])
            {
                auto s = OsmiumSegment(vertices[v], vertices[p[v]]);
                if (generated_segments.contains(s))
                {
                    util::Log(logDEBUG)
                        << "    Collecting: " << s.first.ref() << " -> " << s.second.ref();
                    result.emplace(s);
                }
                v = p[v];
            }
        }
    }
    return result;
}

} // namespace osrm::extractor::area
