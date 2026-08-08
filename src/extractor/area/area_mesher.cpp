// FIXME: include entrances in entry points
// FIXME: include crossing roads inside area

#include "extractor/area/area_mesher.hpp"

#include "extractor/area/dijkstra.hpp"
#include "extractor/area/typedefs.hpp"
#include "extractor/area/util.hpp"
#include "extractor/area/visibility_graph.hpp"
#include "util/log.hpp"
#include "util/typedefs.hpp"

#include <filesystem>
#include <osmium/builder/attr.hpp>
#include <osmium/builder/osm_object_builder.hpp>
#include <osmium/io/xml_output.hpp>
#include <osmium/memory/buffer.hpp>
#include <osmium/memory/collection.hpp>
#include <osmium/osm/entity.hpp>
#include <osmium/osm/item_type.hpp>
#include <osmium/osm/location.hpp>
#include <osmium/osm/node_ref.hpp>
#include <osmium/osm/relation.hpp>
#include <osmium/osm/types.hpp>

#include <algorithm>
#include <iterator>

namespace osrm::extractor::area
{

namespace
{

/**
 * @brief Copies tags from the area to the generated ways.
 *
 * - removes the `area` tag
 * - adds an `osrm:virtual` tag
 */
void copy_tags(osmium::builder::Builder &parent, const osmium::TagList &tags)
{
    osmium::builder::TagListBuilder builder{parent};

    for (const auto &tag : tags)
    {
        if (!std::strcmp(tag.key(), "area"))
        {
            continue;
        }
        builder.add_tag(tag);
    }
    builder.add_tag("osrm:virtual", "yes");
}

/**
 * @brief Return the id of the OSM object that created the area.
 *
 * @param area The area
 * @return The id in the format "r42" or "w69"
 */
std::string area_id(const osmium::Area &area)
{
    std::stringstream strstream;
    strstream << (area.id() % 2 == 1 ? "r" : "w") << area.orig_id();
    return strstream.str();
}

} // namespace

/**
 * @brief Build a boost multi-polygon from an osmium::Area.
 *
 * It is hard to adapt an osmium::Area to a boost::multi_polygon because the latter is a
 * vector of boost::polygon, while libosmium has no equivalent for boost::polygon. So
 * whenever we need to use boost we use a boost polygon model and copy our data into it.
 *
 * Note that the built polygon is an open one.
 */
OsmiumMultiPolygon AreaMesher::area_builder(const osmium::Area &area)
{
    OsmiumMultiPolygon mp;
    for (const osmium::OuterRing &outer : area.outer_rings())
    {
        OsmiumPolygon poly;
        std::copy(outer.cbegin(), outer.cend() - 1, std::back_inserter(poly.outer()));
        for (const osmium::InnerRing &inner : area.inner_rings(outer))
        {
            OsmiumPolygon::ring_type vinner(inner.cbegin(), inner.cend() - 1);
            poly.inners().push_back(vinner);
        }
        mp.emplace_back(poly);
    }
    return mp;
}

/**
 * @brief Prepares the search for intersecting ways.
 *
 * We need to know by which nodes an area can actually be entered or exited.
 * With this knowledge we can substantially reduce the number of ways to generate.
 *
 * This function generates a multi-map from node ids to ways (actually to the index into
 * ExtractionContainers::ways_list).
 */
void AreaMesher::init(const AreaManager &manager, const extractor::ExtractionContainers &containers)
{
    // For all nodes in any of the registered ways, find out which other ways use these
    // nodes.

    const size_t number_of_ways = containers.ways_list.size();
    for (size_t i = 0; i < number_of_ways; ++i)
    {
        auto start = containers.used_node_id_list.begin() + containers.way_node_id_offsets[i];
        auto end = containers.used_node_id_list.begin() + containers.way_node_id_offsets[i + 1];

        for (auto it = start; it != end; ++it)
        {
            if (manager.node_ids.contains(from_alias<osmium::object_id_type>(*it)))
                node_id2way_index.emplace(*it, containers.ways_list[i]);
        }
    }
};

/**
 * @brief Return the nodes by which the area can be entered or exited.
 *
 * For an area to be meshed there must be at least 2 incident ways. If there is only
 * one way crossing the area, or running along the perimeter, the area need not be
 * meshed.
 *
 * If a way joins the perimeter, runs along it, and then leaves it, only two nodes will
 * be reported.
 *
 * Note that an entry point can at the same time be classified as visibility-blocking
 * obstacle, see below.
 */
NodeRefSet AreaMesher::get_entry_points(const OsmiumPolygon &poly)
{
    NodeRefSet entry_nodes;
    std::set<OSMWayID> all_ways;

    const auto &outer = poly.outer();
    std::size_t size = outer.size();

    auto get_ways_crossing_node = [this, &outer](int index) -> std::set<OSMWayID>
    {
        std::set<OSMWayID> result;
        auto [wno_begin, wno_end] =
            node_id2way_index.equal_range(to_alias<OSMNodeID>(outer[index].ref()));
        for (auto wno_iter = wno_begin; wno_iter != wno_end; ++wno_iter)
        {
            result.emplace(wno_iter->second);
        }
        return result;
    };

    std::set<OSMWayID> last_ways = get_ways_crossing_node(0);

    for (std::size_t i = 1; i <= size; ++i)
    {
        std::set<OSMWayID> current_ways = get_ways_crossing_node(i % size);

        // find the difference between current and last ways
        auto first1 = current_ways.begin();
        auto last1 = current_ways.end();
        auto first2 = last_ways.begin();
        auto last2 = last_ways.end();
        int new_ways = 0;
        int gone_ways = 0;

        while (first1 != last1 && first2 != last2)
        {
            if (*first1 < *first2)
            {
                ++new_ways;
                ++first1;
            }
            else if (*first1 > *first2)
            {
                ++gone_ways;
                ++first2;
            }
            else
            {
                ++first1;
                ++first2;
            }
        }

        if (new_ways || first1 != last1)
        {
            // a new way
            entry_nodes.emplace(outer[i % size]);
        }
        if (gone_ways || first2 != last2)
        {
            // eg. a way that was running along the border has left us
            entry_nodes.emplace(outer[(i - 1) % size]);
        }

        last_ways = current_ways;
        all_ways.insert(current_ways.begin(), current_ways.end());
    }
    if (entry_nodes.size() >= 2 and all_ways.size() >= 3)
        return entry_nodes;
    return NodeRefSet{};
}

/**
 * Return the vertices which potentially block visibility.
 *
 * Specifically return:
 *
 * - all convex vertices on the inner rings, and
 * - all reflex vertices on the outer ring.
 *
 * Only these vertices plus the entry points to the area need to be considered when
 * building the visibility graph.  Note that a blocking vertex can at the same time be
 * an entry point.
 */
NodeRefSet AreaMesher::get_obstacle_vertices(const OsmiumPolygon &poly)
{
    NodeRefSet obstacle_vertices;

    for_each_ring(poly,
                  [&](const auto &ring)
                  {
                      for_each_triplet_in_ring(ring,
                                               [&](const osmium::NodeRef &prev,
                                                   const osmium::NodeRef &n,
                                                   const osmium::NodeRef &next)
                                               {
                                                   if (right(&prev, &n, &next))
                                                   {
                                                       obstacle_vertices.emplace(n);
                                                   }
                                               });
                  });

    util::Log(logDEBUG) << "Obstacle vertices: " << obstacle_vertices.size();
    return obstacle_vertices;
}

/**
 * @brief Meshes one area into a buffer.
 *
 * @param area       The area to mesh
 * @param out_buffer The buffer to receive the ways of the meshed area
 * @param relations  The registry of relations
 */
void AreaMesher::mesh_area(const osmium::Area &area,
                           osmium::memory::Buffer &out_buffer,
                           ExtractionRelationContainer &relations)
{
    util::Log(logDEBUG) << "Meshing area: " << area.get_value_by_key("name", "noname")
                        << " id: " << area_id(area);

    auto rel_ids = relations.get_relations_for(area);
    util::Log(logDEBUG) << "  Found " << rel_ids.size() << " parent relations.";

    // add the segments to the output buffer
    auto add_to_buffer =
        [&](const std::set<OsmiumSegment> &segments, osmium::memory::Buffer &out_buffer)
    {
        using namespace osmium::builder::attr;

        for (const OsmiumSegment &s : segments)
        {
            util::Log(logDEBUG) << "    Reporting: " << s.first.ref() << " -> " << s.second.ref();
            auto builder = osmium::builder::WayBuilder(out_buffer);
            builder.set_id(next_way_id);
            builder.set_version(1);
            copy_tags(builder, area.tags());
            builder.add_node_refs({s.first.ref(), s.second.ref()});
            for (auto rel_id : rel_ids)
            {
                // if the original item was part of a relation, the generated ways
                // should be part of that relation too, eg. a hiking route crossing a
                // pedestrian area
                relations.add_relation_member(rel_id, next_way_id, osmium::item_type::way);
            }
            --next_way_id;
            ++added_ways;
        }
        out_buffer.commit();
    };

#ifndef NDEBUG
    auto debug_nodes =
        [&](const NodeRefSet &nodes, const char *klass, osmium::memory::Buffer &out_buffer)
    {
        for (const osmium::NodeRef node : nodes)
        {
            auto builder = osmium::builder::NodeBuilder(out_buffer);
            builder.set_id(next_node_id);
            builder.set_version(1);
            builder.add_tags({{"osrm:debug:class", klass}});
            builder.set_location(node.location());
            --next_node_id;
        }
        out_buffer.commit();
    };

    const auto write_debug = [&](const char *basename, const std::set<OsmiumSegment> &segments)
    {
        // write debug file
        std::stringstream strstream;
        strstream << basename << "-" << area_id(area) << ".osm";
        std::filesystem::path path = std::filesystem::temp_directory_path() / strstream.str();
        osmium::io::Writer writer(path, osmium::io::overwrite::allow);
        osmium::memory::Buffer debug_buffer{16 * 1024};
        add_to_buffer(segments, debug_buffer);
        debug_buffer.commit();
        writer(std::move(debug_buffer));
        writer.close();
    };
#endif

    for (const OsmiumPolygon &poly : area_builder(area))
    {
        NodeRefSet entry_points = get_entry_points(poly);
        if (entry_points.size() < 2)
        {
            util::Log(logDEBUG) << "  Poly outer size: " << poly.outer().size();
            util::Log(logDEBUG) << "  Poly inner rings: " << poly.inners().size();
            util::Log(logDEBUG) << "  Not enough entry points: " << entry_points.size();
            continue;
        }

        // The vertices that should be in the visibility map.
        NodeRefSet work_vertices = get_obstacle_vertices(poly);

        util::Log(logDEBUG) << "  Starting VisibilityGraph on area with " << entry_points.size()
                            << " entry points and " << work_vertices.size() << " obstacle points.";

#ifndef NDEBUG
        debug_nodes(entry_points, "entry_points", out_buffer);
        debug_nodes(work_vertices, "obstacle_points", out_buffer);
#endif

        work_vertices.insert(entry_points.begin(), entry_points.end());

        // safety valve
        if (work_vertices.size() > max_vertices)
        {
            util::Log(logWARNING) << "Too many vertices (" << work_vertices.size() << "/"
                                  << max_vertices << ") for meshing. OSM id: " << area_id(area);
            continue;
        }

        // Note: we cannot reduce the area to just the work_vertices. Assume a round
        // place with two entries opposite each other and a fountain in the middle. If
        // we reduce the area to a line between the entries the fountain will block
        // everything.
        VisibilityGraph gv;
        std::set<OsmiumSegment> vis_map = gv.run(poly, work_vertices);
        util::Log(logDEBUG) << "  After running VisibilityGraph we have " << vis_map.size()
                            << " visible edges.";

#ifndef NDEBUG
        write_debug("osrm-area-routing-visgraph-debug", vis_map);
#endif

        // std::set<OsmiumSegment> segments = run_dijkstra(OsmiumPolygon(), vis_map, entry_points);
        std::set<OsmiumSegment> segments = run_dijkstra(poly, vis_map, entry_points);
        util::Log(logDEBUG) << "  After running Dijkstra there are " << segments.size()
                            << " edges left.";
        add_to_buffer(segments, out_buffer);
        out_buffer.commit();

#ifndef NDEBUG
        write_debug("osrm-area-routing-dijkstra-debug", segments);
#endif
    }
};

/**
 * @brief Runs the Dijkstra shortest-path algorithm on the visibility graph.
 *
 * @param poly         The area as polygon
 * @param vis_map      The visibility graph
 * @param entry_points The entry points to the area
 * @return             The resulting ways to add to the router
 */
std::set<OsmiumSegment> AreaMesher::run_dijkstra(const OsmiumPolygon &poly,
                                                 std::set<OsmiumSegment> &vis_map,
                                                 const NodeRefSet &entry_points)
{
    Dijkstra<osmium::NodeRef> d;

    std::set<OsmiumSegment> poly_segments;

    // Add the segments in the polygon.
    for_each_ring(poly,
                  [&](auto &ring)
                  {
                      for_each_pair_in_ring(ring,
                                            [&](const osmium::NodeRef &u, const osmium::NodeRef &v)
                                            {
                                                poly_segments.emplace(OsmiumSegment(u, v));
                                                double weight = bg::distance(u, v);
                                                d.add_edge(u, v, weight);
                                            });
                  });

    util::Log(logDEBUG) << "  The polygon has " << poly_segments.size() << " edges:";
    util::Log(logDEBUG) << "  The vis_map has " << vis_map.size() << " edges:";

    // Add the segments of the visibility graph. Avoid duplicates.
    for (const OsmiumSegment &s : vis_map)
    {
        if (!poly_segments.contains(s))
        {
            double weight = bg::distance(s.first, s.second);
            d.add_edge(s.first, s.second, weight);
            util::Log(logDEBUG) << "    " << s.first.ref() << " -> " << s.second.ref() << " "
                                << weight;
        }
    }

    util::Log(logDEBUG) << "Running Dijkstra on: " << entry_points.size() << " entry points, "
                        << d.num_vertices() << " vertices and " << d.num_edges() << " edges.";

    std::set<OsmiumSegment> result;

    using index_t = size_t;
    for (const osmium::NodeRef &entry_point : entry_points)
    {
        index_t u = d.index_of(entry_point);
        d.run(u);

        const std::vector<index_t> &predecessors(d.get_predecessors());

        // starting from each exit point report all generated edges that are on the
        // shortest path from the entry point

        for (const osmium::NodeRef &exit_point : entry_points)
        {
            util::Log(logDEBUG) << "  Collecting segments from " << entry_point.ref() << " -> "
                                << exit_point.ref();
            index_t v = d.index_of(exit_point);
            while (v != u && v != predecessors.at(v))
            {
                auto s = OsmiumSegment(d.get_vertex(v), d.get_vertex(predecessors.at(v)));
                util::Log(logDEBUG)
                    << "    Collecting: " << s.first.ref() << " -> " << s.second.ref();
                result.emplace(s);
                v = predecessors.at(v);
            }
        }
    }
    return result;
}

/**
 * @brief Meshes all areas in the input buffer.
 *
 * @param in_buffer  The input buffer with areas
 * @param out_buffer The output buffer with ways
 * @param relations  The registry of relations
 */
void AreaMesher::mesh_buffer(const osmium::memory::Buffer &in_buffer,
                             osmium::memory::Buffer &out_buffer,
                             ExtractionRelationContainer &relations)
{
    for (const auto &area : in_buffer.select<osmium::Area>())
    {
        mesh_area(area, out_buffer, relations);
    }
}

/**
 * @brief Return a buffer with areas.
 *
 * Fill the next buffer with up to 4 areas and return it.  An invalid buffer signals
 * that there are no more areas. After that, all calls will throw an
 * @ref osrm::util::exception.
 *
 * @returns A buffer with areas
 * @throws osrm::util::exception if there is an error.
 */
osmium::memory::Buffer BufferReader::read()
{
    if (m_status != status::okay)
    {
        throw osrm::util::exception(
            "extractor::area::BufferReader: cannot read in status 'closed', 'eof', or 'error'");
    }
    if (iter == end)
    {
        m_status = status::eof;
        return osmium::memory::Buffer();
    }

    const size_t no_of_areas_per_buffer = 4;

    osmium::memory::Buffer out_buffer{16 * 1024, osmium::memory::Buffer::auto_grow::yes};
    size_t i = 0;

    while (true)
    {
        if (iter->type() == osmium::item_type::area)
        {
            out_buffer.add_item(*iter);
            ++i;
        }
        ++iter;
        if (iter == end || i >= no_of_areas_per_buffer)
        {
            out_buffer.commit();
            return out_buffer;
        }
    }
}

} // namespace osrm::extractor::area
