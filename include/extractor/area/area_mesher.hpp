#ifndef OSRM_EXTRACTOR_AREA_AREA_MESHER_HPP
#define OSRM_EXTRACTOR_AREA_AREA_MESHER_HPP

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

    int added_ways{0};

  private:
    using NodeIDVector = std::vector<OSMNodeID>;
    using WayNodeIDOffsets = std::vector<size_t>;
    using WayIDVector = std::vector<OSMWayID>;

    ExtractionRelationContainer::RelationIDList
    get_relations(const osmium::Area &area, const ExtractionRelationContainer &relations);

    std::unordered_multimap<OSMNodeID, OSMWayID> node_id2way_index;
    osmium::object_id_type next_way_id{(1ULL << 34) - 1}; // see: packed_osm_ids.hpp
#ifndef NDEBUG
    osmium::object_id_type next_node_id{(1ULL << 34) - 1}; // see: packed_osm_ids.hpp
#endif
};

} // namespace osrm::extractor::area

#endif // AREA_MESHER.HPP
