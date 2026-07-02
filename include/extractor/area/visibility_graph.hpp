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

namespace osrm::extractor::area
{
namespace
{
namespace srs = boost::geometry::srs;

srs::projection<srs::static_epsg<3857>> osm_mercator;
} // namespace

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
        Vertex(const osmium::NodeRef &n) : node{n} { osm_mercator.forward(n, *this); };

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
            // sort in clockwise order, shorter distance breaks ties
            return a.angle > b.angle || (a.angle == b.angle && a.distance < b.distance);
        };
        friend bool operator==(const Vertex &a, const Vertex &b) noexcept
        {
            return a.ref() == b.ref();
        }
    };

    /** An edge of the visibilty graph */
    struct Segment
    {
        const Vertex *first;
        const Vertex *second;
        double distance{0};

        Segment(const Vertex *first, const Vertex *second)
            : first{first}, second{second}, distance{0} {};

        friend bool operator<(const Segment &a, const Segment &b) noexcept
        {
            return a.distance < b.distance;
        };
        friend inline bool operator==(const Segment &a, const Segment &b) noexcept
        {
            return (a.first == b.first && a.second == b.second);
        }
    };

    // An open polygon of Vertex
    using VertexPoly = bg::model::polygon<Vertex, false, false>;

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
const double COEFF = 100.0;

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
    using type = bg::cs::cartesian; // Point is projected into mercator
};
template <std::size_t K> struct access<VisibilityGraph::Vertex, K>
{
    static inline double get(const VisibilityGraph::Vertex &v) { return v.point[K] / COEFF; }
    static inline void set(VisibilityGraph::Vertex &v, const double &value)
    {
        v.point[K] = value * COEFF;
    }
};

} // namespace boost::geometry::traits

namespace osrm::extractor::area
{

/**
 * @brief Calculate the visibility graph.
 *
 * For each node in the work_set, and for each vertex of poly visible from that
 * node, it returns the segment from the node to the vertex.
 */
std::set<OsmiumSegment> VisibilityGraph::run(const OsmiumPolygon &poly, NodeRefSet &work_set)
{
    // copy the NodeRef polygon into a Vertex polygon
    VertexPoly vpoly;

    std::transform(poly.outer().begin(),
                   poly.outer().end(),
                   std::back_inserter(vpoly.outer()),
                   [](const auto n) { return Vertex{n}; });
    for (auto &inner : poly.inners())
    {
        bg::model::ring<Vertex, false, false> vinner;
        std::transform(inner.begin(),
                       inner.end(),
                       std::back_inserter(vinner),
                       [](const auto n) { return Vertex{n}; });
        vpoly.inners().push_back(vinner);
    }

    // for each ring, for each vertex, link it to the previous and the next one
    for_each_ring(vpoly,
                  [](auto &ring)
                  {
                      for_each_pair_in_ring(ring,
                                            [](Vertex &v, Vertex &next)
                                            {
                                                v.next = &next;
                                                next.prev = &v;
                                            });
                  });

    // for each node in the working set, find the visible vertices
    std::set<OsmiumSegment> result;
    for (const osmium::NodeRef &observer : work_set)
    {
        for (const Vertex *w : visible_vertices(vpoly, observer))
        {
            if (w->visible && work_set.contains(w->ref()))
            {
                result.emplace(OsmiumSegment(observer, w->toNodeRef()));
            }
        }
    }
    util::Log(logDEBUG) << "Found " << result.size() << " lines of sight.";
    return result;
}

/**
 * @brief Return all vertices of poly that the observer can see.
 *
 * @param poly     the polygon
 * @param observer the vertex where the observer stands
 */
std::vector<VisibilityGraph::Vertex *> VisibilityGraph::visible_vertices(VertexPoly &poly,
                                                                         const Vertex &observer)
{
    util::Log(logDEBUG) << "Calling visible_vertices on node: " << observer.ref();

    // 1. Initialize vertices_cw.
    //    Insert all vertices (except the observer itself) into vertices_cw. Sort
    //    vertices_cw in clockwise order around the observer.

    // The vertices sorted in clockwise order around the observer.
    std::vector<Vertex *> vertices_cw;

    for_each_ring(poly,
                  [&](auto &ring)
                  {
                      for (Vertex &v : ring)
                      {
                          if (v != observer)
                          {
                              vertices_cw.push_back(&v);
                          }
                      }
                  });
    util::Log(logDEBUG) << "Vertices CW: " << vertices_cw.size();

    for (Vertex *v : vertices_cw)
    {
        v->distance = bg::comparable_distance(observer, *v);
        double dx = bg::get<0>(*v) - bg::get<0>(observer);
        double dy = bg::get<1>(*v) - bg::get<1>(observer);
        // See: pseudoangles
        // https://stackoverflow.com/questions/16542042
        // https://computergraphics.stackexchange.com/questions/10522
        v->angle = std::copysign(1. - (dx / (fabs(dx) + fabs(dy))), dy);
    }
    std::sort(vertices_cw.begin(),
              vertices_cw.end(),
              [](const Vertex *a, const Vertex *b) { return *a < *b; });

    util::Log(logDEBUG) << "Sorted vertices_cw:";
    for (const Vertex *v : vertices_cw)
    {
        util::Log(logDEBUG) << "  OSM id: " << v->ref() << " at " << v->angle;
    }

    // 2. Initialize the sweep status tau.
    //
    //    Let rho be a ray starting at the observer and going through the first vertex.
    //    Find all segments that are intersected by rho and store them in tau in the
    //    same order they are intersected by rho. (Actually store them in pending edges
    //    from where they will be later transferred into tau.)
    //
    //    In the textbook the observer lies outside of any obstacles, but our observer
    //    stands on the polygon boundary. So we must take care not to insert edges
    //    adjacent to the observer vertex because those edges will not obscure anything
    //    but interfere with distance ordering.

    // The sweep status. An ordered collection of edges sorted by increasing distance.
    std::vector<Segment> tau;
    // Edges about to be inserted in tau.
    std::deque<Segment> pending_edges;

    const Vertex *q = vertices_cw[0];
    for (const Vertex *v : vertices_cw)
    {
        if (&observer != v->prev && &observer != v->next &&
            intersect(&observer, q, v, v->next, (Vertex *)nullptr, true))
        {
            pending_edges.emplace_back(v, v->next);
        }
    };
    util::Log(logDEBUG) << "Starting sweep with " << pending_edges.size() << " pending edges.";

    // 3. Sweep the ray rho clockwise and stop at each vertex.
    //    rho is implicitly defined by: observer -> w -> infinity
    //    At each vertex do:
    //    - insert pending edges into tau in order of distance
    //    - test the visibility of the vertex
    //    - remove from tau any incident edge to the ccw of the sweep ray
    //    - add to the pending edges any incident edge to the cw of the sweep ray
    //

    // The previously visited vertex in CW order.
    const Vertex *prev_w = nullptr;
    for (Vertex *w : vertices_cw)
    {
        util::Log(logDEBUG) << "Now at node id:" << w->ref() << " with " << tau.size()
                            << " edges in tau";

        // update tau with pending edges
        if (!pending_edges.empty())
        {
            // update the distances of the segments in the status (the order of the
            // segments does not change because segments are not supposed to intersect)
            Vertex intersection;
            for (Segment &s : tau)
            {
                if (intersect(&observer, w, s.first, s.second, &intersection, true))
                {
                    s.distance = bg::comparable_distance(observer, intersection);
                    util::Log(logDEBUG)
                        << "  Updated distance of edge id:" << s.first->ref()
                        << " -> id:" << s.second->ref() << " in tau to: " << s.distance;
                }
            };
            // insert pending edges into tau in the correct order
            while (!pending_edges.empty())
            {
                Segment &s = pending_edges[0];
                util::Log(logDEBUG) << "  Pending edge unstashed: id:" << s.first->ref()
                                    << " -> id:" << s.second->ref();
                util::Log(logDEBUG)
                    << "               intersection test with: id:" << observer.ref()
                    << " -> id:" << w->ref();
                if (intersect(&observer, w, s.first, s.second, &intersection, true))
                {
                    s.distance = bg::comparable_distance(observer, intersection);
                    auto at = tau.insert(std::lower_bound(tau.begin(), tau.end(), s), s);
                    util::Log(logDEBUG)
                        << "               and inserted into tau at pos "
                        << std::distance(tau.begin(), at) << " and distance " << s.distance;
                }
                else
                {
                    util::Log(logDEBUG) << "               and dropped";
                }
                pending_edges.pop_front();
            }
        }

        // test visibility p -> w
        util::Log(logDEBUG) << "  Testing visibility of node id:" << w->ref() << " with "
                            << tau.size() << " edges in tau.";
        for (Segment &s : tau)
        {
            util::Log(logDEBUG) << "    Edge id:" << s.first->ref() << " -> id:" << s.second->ref()
                                << " distance: " << s.distance;
        }
        w->visible = visible(&observer, prev_w, w, tau);
        prev_w = w;
        util::Log(logDEBUG) << "    Result: " << w->visible;

        // update tau
        //
        // Delete the old edges to the CCW (left) of the sweep ray. Then insert the new
        // edges to the CW (right) of the sweep ray. There is a difficulty here that
        // the textbook elegantly glosses over: tau is supposed to store the intersected
        // edges in order of increasing distance, but if there are *two* new edges to
        // insert their order is not apparent yet. Our only choice is to handle those
        // edges later.
        auto update_tau = [&](const Vertex *u, const Vertex *v, bool left)
        {
            if (left)
            {
                util::Log(logDEBUG) << "  Erasing edge: id:" << u->ref() << " -> id:" << v->ref();
                std::erase_if(tau,
                              [u, v](const Segment &s)
                              {
                                  bool delendus = (*s.first == *u) && (*s.second == *v);
                                  if (delendus)
                                      util::Log(logDEBUG)
                                          << "  Edge erased from tau: id:" << u->ref()
                                          << " -> id:" << v->ref();
                                  return delendus;
                              });
            }
            else
            {
                pending_edges.emplace_back(u, v);
                util::Log(logDEBUG)
                    << "  Pending edge stashed: id:" << u->ref() << " -> id:" << v->ref();
            }
        };
        update_tau(w->prev, w, leftOrOn(&observer, w, w->prev));
        update_tau(w, w->next, leftOrOn(&observer, w, w->next));
    }
    return vertices_cw; // visible vertices have "visible" set
}

/**
 * @brief Return true if the observer can see vertex w.
 *
 * @param observer the observer at the center of the circular clockwise sweep
 * @param prev_w   the previous vertex in clockwise order around the sweep center
 * @param w        the current vertex
 * @param tau      maintains the sweep status
 */
bool VisibilityGraph::visible(const Vertex *observer,
                              const Vertex *prev_w, // the last visited vertex
                              const Vertex *w,
                              const std::vector<Segment> &tau)
{
    // Check if observer -> w is inside the polygon immediately before intersecting the
    // edge.  This condition also checks if the ray falls entirely outside the outer
    // ring.
    //
    // if observer -> w intersects the interior of the obstacle of which w is a
    // vertex, locally at w then w is not visible
    if (in_open_cone(w->next, w, w->prev, observer))
    {
        util::Log(logDEBUG) << "    Invisible because in cone";
        return false;
    }

    // Handle the simple case first: the ray really did turn some since the last vertex
    //
    // if prev_w is not on the ray observer -> w
    if (!prev_w || !collinear(observer, prev_w, w))
    {
        // If there is an edge in tau and observer -> w intersects it, then w is not visible.
        if (tau.size() > 0 && intersect(observer, w, tau[0].first, tau[0].second))
        {
            util::Log(logDEBUG) << "    Invisible because obstructed by tau[0]";
            return false;
        }
        return true;
    }

    // The special cases follow: the ray did not turn because the last two vertices are
    // collinear with the observer

    // if prev_w was not visible, then w is not visible either, because w is farther
    // away from the observer
    if (!prev_w->visible)
    {
        return false;
    }

    // if prev_w was visible, search for any edge that obstructs prev_w -> w
    for (const Segment &s : tau)
    {
        if (intersect(prev_w, w, s.first, s.second))
            return false;
    }
    return true;
}

} // namespace osrm::extractor::area

#endif // OSRM_EXTRACTOR_AREA_VISIBILITY_GRAPH_HPP
