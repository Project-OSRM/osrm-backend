#ifndef OSRM_EXTRACTOR_AREA_VISIBILITY_GRAPH_HPP
#define OSRM_EXTRACTOR_AREA_VISIBILITY_GRAPH_HPP

#include "typedefs.hpp"
#include "util.hpp"
#include "util/log.hpp"

#include <boost/geometry/algorithms/comparable_distance.hpp>
#include <boost/geometry/geometries/polygon.hpp>

#include <osmium/osm/node_ref.hpp>
#include <osmium/osm/types.hpp>

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <set>
#include <unordered_map>
#include <vector>

namespace osrm::extractor::area
{

/**
 * @brief Implements the visibility graph
 *
 * This implementation follows the outline in Chapter 15 of: *de Berg, Cheong, van
 * Kreveld, Overmars. Computational Geometry. Third Edition. Springer 2008.*
 *
 * tldr: To compute the visibility graph we do a circular sweep around the observer. We
 * maintain the sweep status in tau. Edges are not supposed to cross each other.
 *
 * Define "interesting" as a vertex that either:
 * - is an entrance to the area,
 * - is reflex (sticks into the area).
 *
 * For each interesting vertex in turn:
 * - put the observer at the vertex
 * - sort all other vertices in clockwise order around the observer
 * - shoot a ray from the observer through the first vertex and initialize the status in tau
 * - sweep the ray clockwise around the observer and
 * - for each vertex intersected by the ray
 *   - report the vertex if it is visible
 *   - update the status in tau
 *
 * Deviation from the textbook: the textbook keeps tau ordered by distance along the
 * sweep ray and tests only its nearest edge.  We keep tau unordered and test *every*
 * edge in it instead.  Both answer the same question -- "does observer -> w cross an
 * obstacle edge that the ray currently passes through" -- but the unordered variant does
 * not depend on distances that cannot be maintained reliably: whenever the sweep ray
 * runs exactly through a vertex (which happens at every single sweep step, and again
 * whenever three vertices are collinear with the observer) the ray/segment intersection
 * is rejected as an endpoint hit and the edge keeps a stale distance.  A single stale
 * distance is enough to put a non-blocking edge in front, and the textbook test then
 * reports a vertex as visible although a farther edge in tau does block it.  tau holds
 * only the edges the ray currently crosses -- a handful even for large polygons -- so
 * scanning it is not a meaningful cost.
 */
class VisibilityGraph
{
  public:
    /** A Vertex of the visibility graph */
    struct Vertex
    {
        // The std::sort code expects a default constructor, using pointers here instead
        // of references allows the compiler to generate one.
        Vertex() = default;
        Vertex(const osmium::NodeRef &n);

        osmium::object_id_type ref() const noexcept { return node.ref(); };
        const osmium::NodeRef &toNodeRef() const noexcept { return node; };

        /** Projected coordinates X,Y */
        int64_t point[2]{0, 0};

        const Vertex *prev{nullptr};
        const Vertex *next{nullptr};

        double angle{0.0};
        double distance{0.0};
        bool visible{false}; // is the vertex visible

      private:
        osmium::NodeRef node;

        friend bool operator<(const Vertex &a, const Vertex &b) noexcept
        {
            // Sort in clockwise order, shorter distance breaks ties, then node-id.
            // Node-id makes the order deterministic when angle and distance are equal.
            //
            // The comparisons are exact on purpose.  A tolerant comparison ("equal if
            // within epsilon") is not transitive and therefore not a strict weak
            // ordering, which makes std::sort undefined behaviour.  Comparing exactly
            // and falling back to the node-id yields a total order.
            if (a.angle != b.angle)
                return a.angle > b.angle;
            if (a.distance != b.distance)
                return a.distance < b.distance;
            return a.ref() < b.ref();
        };
        friend bool operator==(const Vertex &a, const Vertex &b) noexcept
        { return a.ref() == b.ref(); }
    };

    /** An edge of the visibilty graph */
    struct Segment
    {
        const Vertex *first;
        const Vertex *second;

        Segment(const Vertex *first, const Vertex *second) : first{first}, second{second} {};

        friend inline bool operator==(const Segment &a, const Segment &b) noexcept
        { return (a.first == b.first && a.second == b.second); }
    };

    // An open polygon of Vertex
    using VertexPoly = boost::geometry::model::polygon<Vertex, false, false>;

    std::set<OsmiumSegment> run(const OsmiumPolygon &poly, NodeRefSet &work_set);

    std::vector<Vertex *> visible_vertices(VertexPoly &poly, const Vertex &observer);

    bool visible(const Vertex *observer,
                 const Vertex *prev_w,
                 const Vertex *w,
                 const std::vector<Segment> &tau);
};

} // namespace osrm::extractor::area

namespace boost::geometry::traits
{

using namespace osrm::extractor::area;

// The coordinates are projected into OSM-Mercator and then multipiled by COEFF so to
// store them in a pair of int64_t.  Integer arithmetic avoids a host of nasty bugs.
inline constexpr double COEFF = 100.0;

template <> struct tag<VisibilityGraph::Vertex>
{
    using type = point_tag;
};
template <> struct dimension<VisibilityGraph::Vertex> : boost::mpl::int_<2>
{
};
template <> struct coordinate_type<VisibilityGraph::Vertex>
{
    using type = double;
};
template <> struct coordinate_system<VisibilityGraph::Vertex>
{
    using type = boost::geometry::cs::cartesian; // Point is projected into mercator
};
template <std::size_t K> struct access<VisibilityGraph::Vertex, K>
{
    static inline double get(const VisibilityGraph::Vertex &v) { return v.point[K] / COEFF; }
    static inline void set(VisibilityGraph::Vertex &v, const double &value)
    { v.point[K] = value * COEFF; }
};

} // namespace boost::geometry::traits

#endif // OSRM_EXTRACTOR_AREA_VISIBILITY_GRAPH_HPP
