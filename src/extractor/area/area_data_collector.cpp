#include "extractor/area/area_data_collector.hpp"

#include <algorithm>
#include <iterator>
#include <tuple>

namespace osrm::extractor::area
{

void AreaDataCollector::record(const OsmiumPolygon &poly, double walking_speed)
{
    PolygonRecord record;
    record.boundary_vertices.assign(poly.outer().begin(), poly.outer().end());
    record.obstacle_rings.reserve(poly.inners().size());
    for (const auto &inner : poly.inners())
        record.obstacle_rings.emplace_back(inner.begin(), inner.end());
    record.walking_speed = walking_speed;

    tbb::mutex::scoped_lock lock(m_mutex);
    m_polygons.push_back(std::move(record));
}

void AreaDataCollector::finalize()
{
    // Order by the node ids of the outer ring.  The ring is a rotation-stable list of
    // ids for a given area, so this is a property of the input rather than of the run.
    const auto key = [](const PolygonRecord &r)
    {
        std::vector<osmium::object_id_type> ids;
        ids.reserve(r.boundary_vertices.size());
        std::transform(r.boundary_vertices.begin(),
                       r.boundary_vertices.end(),
                       std::back_inserter(ids),
                       [](const osmium::NodeRef &n) { return n.ref(); });
        return ids;
    };

    std::sort(m_polygons.begin(),
              m_polygons.end(),
              [&key](const PolygonRecord &a, const PolygonRecord &b) { return key(a) < key(b); });
}

} // namespace osrm::extractor::area
