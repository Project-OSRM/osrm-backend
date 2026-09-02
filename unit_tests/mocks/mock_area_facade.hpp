#ifndef MOCK_AREA_FACADE_HPP
#define MOCK_AREA_FACADE_HPP

// A facade carrying a handful of open areas, for the snapping and geodesic code.  The
// real one answers from an r-tree over bounding boxes and a flat vertex array; this
// answers the same three questions from a vector of polygons.

#include "mocks/mock_datafacade.hpp"

#include "extractor/area_routing_data.hpp"
#include "engine/phantom_node.hpp"
#include "util/coordinate_calculation.hpp"

#include <algorithm>
#include <optional>
#include <span>
#include <vector>

namespace osrm::test
{

// A degree of longitude at the equator, near enough for a fixture: the numbers are laid
// out in metres and converted, the same convention as unit_tests/engine/area_geodesic.cpp.
constexpr double AREA_METRE = 1.0 / 111319.49;

inline util::Coordinate at(double x, double y)
{ return {util::FloatLongitude{x * AREA_METRE}, util::FloatLatitude{y * AREA_METRE}}; }

/**
 * One area, remembered with whether the mesher gave its vertices a way.
 *
 * An area with no reachable vertex is not a contrivance.  The mesher declines anything
 * over AreaMesher::max_vertices, and the record into `.osrm.openareas` happens before that
 * test, so a large plaza is stored with no mesh behind it.  Every vertex of it then fails
 * the "is there a phantom standing here" question, which is what `reachable` false stands
 * for.
 */
struct MockArea
{
    std::vector<util::Coordinate> outer;
    extractor::AreaPolygonSegment segment;
    bool reachable = true;
};

// MockDataFacade is final, and the area code only ever takes a BaseDataFacade, so the
// base half of the mock is all this needs.
class MockAreaFacade final : public MockBaseDataFacade
{
  public:
    std::vector<MockArea> areas;

    /** Bounding boxes, as the r-tree gives them: a hit here is not containment. */
    std::vector<extractor::AreaPolygonSegment>
    GetOpenAreasAt(const util::Coordinate coordinate) const override
    {
        std::vector<extractor::AreaPolygonSegment> found;
        for (const auto &area : areas)
        {
            if (within_bounding_box(area, coordinate))
            {
                found.push_back(area.segment);
            }
        }
        return found;
    }

    std::vector<std::span<const util::Coordinate>>
    GetOpenAreaRings(const extractor::AreaPolygonSegment &segment) const override
    {
        for (const auto &area : areas)
        {
            if (area.segment.vertices_offset == segment.vertices_offset)
            {
                return {std::span<const util::Coordinate>{area.outer}};
            }
        }
        return {};
    }

    /**
     * A phantom stands on a vertex exactly when the mesher gave that vertex a way.  The
     * segment ids have to be distinct per vertex, because snapping claims each one once.
     */
    std::vector<engine::PhantomNodeWithDistance>
    NearestPhantomNodes(const util::Coordinate coordinate,
                        const size_t /*max_results*/,
                        const std::optional<double> /*max_distance*/,
                        const std::optional<engine::Bearing> /*bearing*/,
                        const engine::Approach /*approach*/) const override
    {
        std::vector<engine::PhantomNodeWithDistance> found;
        NodeID id = 1;
        for (const auto &area : areas)
        {
            for (const auto vertex : area.outer)
            {
                if (area.reachable &&
                    util::coordinate_calculation::greatCircleDistance(vertex, coordinate) < 0.5)
                {
                    found.push_back({standing_at(vertex, id), 0.0});
                }
                id += 2;
            }
        }
        return found;
    }

    /** The same phantom NearestPhantomNodes would find, by vertex rather than by search. */
    std::vector<engine::PhantomNodeWithDistance>
    PhantomNodesOnAreaVertex(const extractor::AreaPolygonSegment &segment,
                             const std::uint32_t vertex,
                             const engine::Approach /*approach*/) const override
    {
        NodeID id = 1;
        for (const auto &area : areas)
        {
            if (area.segment.vertices_offset == segment.vertices_offset)
            {
                if (!area.reachable || vertex >= area.outer.size())
                {
                    return {};
                }
                return {{standing_at(area.outer[vertex], id + 2 * vertex), 0.0}};
            }
            id += 2 * area.outer.size();
        }
        return {};
    }

  private:
    static bool within_bounding_box(const MockArea &area, const util::Coordinate coordinate)
    {
        auto low = area.outer.front(), high = area.outer.front();
        for (const auto vertex : area.outer)
        {
            low.lon = std::min(low.lon, vertex.lon);
            low.lat = std::min(low.lat, vertex.lat);
            high.lon = std::max(high.lon, vertex.lon);
            high.lat = std::max(high.lat, vertex.lat);
        }
        return coordinate.lon >= low.lon && coordinate.lon <= high.lon &&
               coordinate.lat >= low.lat && coordinate.lat <= high.lat;
    }

    /**
     * A phantom sitting on a segment's first node: no weight forwards, the whole segment
     * in reverse.  That is the shape snapping looks for when a traveller departs.
     */
    static engine::PhantomNode standing_at(const util::Coordinate vertex, const NodeID id)
    {
        // the constructor is the only way in: is_valid_* are private bitfields
        struct Segment
        {
            SegmentID forward_segment_id;
            SegmentID reverse_segment_id;
            unsigned short fwd_segment_position;
        } segment{{id, true}, {id + 1, true}, 0};

        return engine::PhantomNode{segment,
                                   ComponentID{0, 0},
                                   EdgeWeight{0},
                                   EdgeWeight{100},
                                   EdgeWeight{0},
                                   EdgeWeight{0},
                                   EdgeDistance{0},
                                   EdgeDistance{100},
                                   EdgeDistance{0},
                                   EdgeDistance{0},
                                   EdgeDuration{0},
                                   EdgeDuration{100},
                                   EdgeDuration{0},
                                   EdgeDuration{0},
                                   true,
                                   true,
                                   true,
                                   true,
                                   vertex,
                                   vertex,
                                   0};
    }
};

/** A square with its low corner at (lo, lo) and its high corner at (hi, hi). */
inline MockArea mock_square(double lo, double hi, std::uint32_t offset, bool reachable = true)
{
    MockArea area;
    area.outer = {at(lo, lo), at(hi, lo), at(hi, hi), at(lo, hi)};
    area.segment.vertices_offset = offset;
    area.segment.num_vertices = 4;
    area.segment.num_rings = 1;
    area.segment.walking_speed = 1.4;
    area.reachable = reachable;
    return area;
}

/**
 * A right triangle on the same bounding box as `mock_square(lo, hi, ...)`.
 *
 * Its bounding box therefore claims coordinates it does not contain, which is the case
 * that separates "the r-tree filed it here" from "it is inside".
 */
inline MockArea mock_triangle(double lo, double hi, std::uint32_t offset, bool reachable = true)
{
    MockArea area;
    area.outer = {at(lo, lo), at(hi, lo), at(lo, hi)};
    area.segment.vertices_offset = offset;
    area.segment.num_vertices = 3;
    area.segment.num_rings = 1;
    area.segment.walking_speed = 1.4;
    area.reachable = reachable;
    return area;
}

} // namespace osrm::test

#endif // MOCK_AREA_FACADE_HPP
