#include "extractor/area/area_data_collector.hpp"

#include <algorithm>
#include <iterator>
#include <tuple>
#include <unordered_map>

namespace osrm::extractor::area
{

std::size_t AreaDataCollector::record(const OsmiumPolygon &poly, double walking_speed)
{
    PolygonRecord record;
    record.boundary_vertices.assign(poly.outer().begin(), poly.outer().end());
    record.obstacle_rings.reserve(poly.inners().size());
    for (const auto &inner : poly.inners())
        record.obstacle_rings.emplace_back(inner.begin(), inner.end());
    record.walking_speed = walking_speed;

    tbb::mutex::scoped_lock lock(m_mutex);
    m_polygons.push_back(std::move(record));
    return m_polygons.size() - 1;
}

void AreaDataCollector::add_visibility(std::size_t handle, const std::set<OsmiumSegment> &edges)
{
    tbb::mutex::scoped_lock lock(m_mutex);
    auto &record = m_polygons.at(handle);

    // node id -> flat index, the order WriteOpenAreas will lay the vertices out in
    std::unordered_map<osmium::object_id_type, std::uint32_t> index_of;
    std::uint32_t index = 0;
    for (const auto &node : record.boundary_vertices)
        index_of.emplace(node.ref(), index++);
    for (const auto &ring : record.obstacle_rings)
        for (const auto &node : ring)
            index_of.emplace(node.ref(), index++);

    record.visibility.assign(index, {});
    for (const auto &edge : edges)
    {
        const auto u = index_of.find(edge.first.ref()), v = index_of.find(edge.second.ref());
        if (u == index_of.end() || v == index_of.end() || u->second == v->second)
            continue;
        record.visibility[u->second].push_back(v->second);
        record.visibility[v->second].push_back(u->second);
    }
    // a property of the data, not of the set's iteration order
    for (auto &targets : record.visibility)
        std::sort(targets.begin(), targets.end());
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
