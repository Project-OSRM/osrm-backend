#ifndef OSRM_EXTRACTOR_AREA_AREA_MESHER_HPP
#define OSRM_EXTRACTOR_AREA_AREA_MESHER_HPP

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

    int added_ways{0};
    /** Refuse to mesh more vertices */
    size_t max_vertices{100};

  private:
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
