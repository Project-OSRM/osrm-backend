#ifndef OSRM_EXTRACTOR_AREA_AREA_MESHER_HPP
#define OSRM_EXTRACTOR_AREA_AREA_MESHER_HPP

#include "extractor/area/area_data_collector.hpp"
#include "extractor/extraction_relation.hpp"
#include "typedefs.hpp"

#include "extractor/extraction_containers.hpp"
#include "util/typedefs.hpp"

#include <osmium/osm/area.hpp>
#include <osmium/osm/types.hpp>

#include <unordered_map>
#include <vector>

namespace osmium
{
namespace memory
{
class Buffer;
};

}; // namespace osmium

namespace osrm::extractor::area
{

class AreaManager;

/**
 * @brief A class that "meshes" areas
 *
 * This class "meshes" an area by creating OSM ways for each shortest path between all
 * pairs of entry points to the area. It first generates a @ref VisibilityGraph
 * "visibility map", then uses @ref Dijkstra "Dijkstra's shortest-path algorithm" to
 * reduce the number of edges. The generated ways are returned in an
 * osmium::memory::Buffer.
 */
class AreaMesher
{
  public:
    void init(const AreaManager &manager, const extractor::ExtractionContainers &containers);
    OsmiumMultiPolygon area_builder(const osmium::Area &area);
    NodeRefSet get_entry_points(const OsmiumPolygon &poly);
    NodeRefSet get_obstacle_vertices(const OsmiumPolygon &poly);
    void mesh_area(const osmium::Area &area,
                   osmium::memory::Buffer &out_buffer,
                   ExtractionRelationContainer &relations);

    void mesh_buffer(const osmium::memory::Buffer &in_buffer,
                     osmium::memory::Buffer &out_buffer,
                     ExtractionRelationContainer &relations);
    osmium::memory::Buffer read();

    /** What the engine needs to snap into these areas later. */
    AreaDataCollector &collector() { return m_collector; }
    const AreaDataCollector &collector() const { return m_collector; }

    int added_ways{0};
    /**
     * The most obstacle-plus-entry vertices an area may have and still be meshed.
     *
     * A safety valve against degenerate input, not a routing-quality knob. The engine
     * used to rebuild the visibility graph per request, cubic in the vertex count, and
     * this bounded that; now the graph is written at extraction (Extractor::WriteOpenAreas)
     * and the cost is paid once. Over all of Ile-de-France the sweep is 4.8 s and the
     * largest real plaza -- Place de la Republique, 1044 vertices -- is 711 work vertices
     * and 0.6 s. 1024 keeps every real plaza there and stops only genuinely pathological
     * geometry, whose sweep grows with work x vertices.
     */
    size_t max_vertices{1024};
    /** Speed in m/s for crossing an area, taken from the profile. */
    double area_walking_speed{1.4};
    /**
     * Effective area, in square metres, below which a vertex is simplified away.
     *
     * An OSM plaza is drawn to be looked at: its outline follows kerbstones at a
     * resolution nothing downstream can use, and every vertex is paid for again on each
     * query that snaps into the area.  Zero switches it off.  See area/simplify.hpp.
     */
    double area_simplify_threshold{0.0};
    /**
     * Emit the area's whole visibility graph rather than the pruned mesh.
     *
     * run_dijkstra() keeps the shortest-path tree rooted at each entry point, which holds
     * a shortest path from every vertex to every entry point.  That is all a coordinate
     * inside the area needs -- it sets off towards a vertex it can see and then wants the
     * shortest way out -- so the pruned mesh is exactly as good as the whole graph for
     * any journey with one end at an entry point, at O(entry points x vertices) rather
     * than O(vertices squared).
     *
     * The one thing it cannot promise is a journey that begins and ends inside the same
     * area, which wants a path between two arbitrary vertices.  The engine no longer asks
     * the mesh for that: Extractor::WriteOpenAreas stores the whole visibility graph
     * beside the mesh and the geodesic is solved over it.  So this is a debugging knob --
     * it shows the graph as ways one can look at -- and not the way to serve that
     * journey, which costs a way per line of sight where storing it costs two integers.
     *
     * Either way the ring edges are emitted -- see with_ring_edges().  See docs/areas.md.
     */
    bool emit_visibility_graph{false};

  private:
    AreaDataCollector m_collector;

    using NodeIDVector = std::vector<OSMNodeID>;
    using WayNodeIDOffsets = std::vector<size_t>;
    using WayIDVector = std::vector<OSMWayID>;

    std::set<OsmiumSegment> run_dijkstra(const OsmiumPolygon &poly,
                                         std::set<OsmiumSegment> &vis_map,
                                         const NodeRefSet &entry_points);

    osmium::object_id_type get_relations(const osmium::Area &area,
                                         const ExtractionRelationContainer &relations);

    std::unordered_multimap<OSMNodeID, OSMWayID> node_id2way_index;
    osmium::object_id_type next_way_id{(1ULL << 34) - 1}; // see: packed_osm_ids.hpp
#ifndef NDEBUG
    osmium::object_id_type next_node_id{(1ULL << 34) - 1}; // see: packed_osm_ids.hpp
#endif
};

/**
 * @brief Implements a reader for a buffer.
 *
 * This class allows you to read from a osmium::memory::Buffer in the same way you
 * would read an OSM file using a osmium::io::Reader.
 */
class BufferReader
{
    osmium::memory::Buffer::const_iterator iter;
    const osmium::memory::Buffer::const_iterator end;
    enum class status
    {
        okay = 0,   // normal reading
        error = 1,  // some error occurred while reading
        closed = 2, // close() called
        eof = 3     // eof of file was reached without error
    };
    status m_status{status::okay};

  public:
    BufferReader(const osmium::memory::Buffer &in_buffer)
        : iter{in_buffer.cbegin()}, end{in_buffer.cend()} {};

    osmium::memory::Buffer read();
};

} // namespace osrm::extractor::area

#endif // AREA_MESHER.HPP
