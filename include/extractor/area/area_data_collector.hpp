#ifndef OSRM_EXTRACTOR_AREA_AREA_DATA_COLLECTOR_HPP
#define OSRM_EXTRACTOR_AREA_AREA_DATA_COLLECTOR_HPP

#include "typedefs.hpp"

#include <oneapi/tbb/mutex.h>

#include <osmium/osm/node_ref.hpp>

#include <vector>

namespace osrm::extractor::area
{

/**
 * @brief What the engine needs to know about one meshed area.
 *
 * Recorded while meshing, because that is the only point where the polygon and its
 * entry points are both known.  Everything here is still in OSM terms; the translation
 * to NodeIDs happens later, once the node id map exists.
 */
struct PolygonRecord
{
    //! The outer ring, open, in the order libosmium delivered it.
    std::vector<osmium::NodeRef> boundary_vertices;
    //! The obstacle rings, open.  Snapping needs their corners: they are the vertices a
    //! coordinate inside the area can see and route around.
    std::vector<std::vector<osmium::NodeRef>> obstacle_rings;
    //! Speed in m/s for crossing this area.
    double walking_speed = 0.0;
};

/**
 * @brief Collects a PolygonRecord per meshed area.
 *
 * Areas are meshed from a tbb::filter_mode::parallel pipeline filter sharing one
 * AreaMesher, so record() is called from several threads at once and takes a lock.
 *
 * That also means the order in which areas arrive is whatever the scheduler decided, and
 * the order determines the index by which the engine will later refer to an area.  Call
 * finalize() once meshing is over to put the records into a deterministic order; two
 * runs over the same input must produce the same file, or the extracted data stops being
 * reproducible and every downstream comparison becomes untrustworthy.
 */
class AreaDataCollector
{
  public:
    void record(const OsmiumPolygon &poly, double walking_speed);

    /** Sort into a deterministic order.  Call once, after meshing. */
    void finalize();

    std::vector<PolygonRecord> &polygons() { return m_polygons; }
    const std::vector<PolygonRecord> &polygons() const { return m_polygons; }
    std::size_t size() const { return m_polygons.size(); }
    void clear() { m_polygons.clear(); }

  private:
    std::vector<PolygonRecord> m_polygons;
    tbb::mutex m_mutex;
};

} // namespace osrm::extractor::area

#endif // OSRM_EXTRACTOR_AREA_AREA_DATA_COLLECTOR_HPP
