#include "extractor/area/visibility_graph.hpp"

#include "extractor/area/util.hpp"
#include "util/log.hpp"

#include <boost/geometry/algorithms/comparable_distance.hpp>

#include <algorithm>
#include <cmath>
#include <iterator>
#include <unordered_map>
#include <vector>

namespace osrm::extractor::area
{
namespace
{
// The projection is stateless; it lives here rather than in the header so that every
// translation unit including the header does not get a copy of its own.
boost::geometry::srs::projection<boost::geometry::srs::static_epsg<3857>> osm_mercator;
} // namespace

VisibilityGraph::Vertex::Vertex(const osmium::NodeRef &n) : node{n}
{ osm_mercator.forward(n, *this); }

} // namespace osrm::extractor::area

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
        boost::geometry::model::ring<Vertex, false, false> vinner;
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

    // index the polygon's vertices by node id.  The observer must be one of them: the
    // sweep compares its address against the ring links to recognize the edges adjacent
    // to the observer, which only works if we hand it the vertex that is actually part
    // of the polygon (an implicitly converted copy would compare unequal to all of them).
    std::unordered_map<osmium::object_id_type, const Vertex *> vertex_by_id;
    for_each_ring(vpoly,
                  [&](auto &ring)
                  {
                      for (const Vertex &v : ring)
                      {
                          vertex_by_id.emplace(v.ref(), &v);
                      }
                  });

    // for each node in the working set, find the visible vertices
    std::set<OsmiumSegment> result;
    for (const osmium::NodeRef &observer : work_set)
    {
        const auto found = vertex_by_id.find(observer.ref());
        if (found == vertex_by_id.end())
        {
            util::Log(logWARNING) << "Observer node " << observer.ref()
                                  << " is not a vertex of the area, skipping.";
            continue;
        }
        for (const Vertex *w : visible_vertices(vpoly, *found->second))
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
        v->distance = boost::geometry::comparable_distance(observer, *v);
        double dx = boost::geometry::get<0>(*v) - boost::geometry::get<0>(observer);
        double dy = boost::geometry::get<1>(*v) - boost::geometry::get<1>(observer);
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
    //    Find all segments that are intersected by rho and store them in tau.
    //
    //    In the textbook the observer lies outside of any obstacles, but our observer
    //    stands on the polygon boundary. So we must take care not to insert edges
    //    adjacent to the observer vertex: those cannot obscure anything, they only meet
    //    the sweep ray in its origin.

    // The sweep status: the obstacle edges the sweep ray currently crosses.  Held in no
    // particular order, see the note on the class.
    std::vector<Segment> tau;

    const Vertex *q = vertices_cw[0];
    for (const Vertex *v : vertices_cw)
    {
        // The edge under consideration is (v, v->next), so it is adjacent to the
        // observer exactly when v->next is the observer -- v->prev being the observer
        // makes v the observer's *successor*, whose outgoing edge does not touch the
        // observer at all and is a perfectly ordinary obstacle.
        if (&observer != v->next && intersect(&observer, q, v, v->next, (Vertex *)nullptr, true))
        {
            tau.emplace_back(v, v->next);
        }
    };
    util::Log(logDEBUG) << "Starting sweep with " << tau.size() << " edges in tau.";

    // 3. Sweep the ray rho clockwise and stop at each vertex.
    //    rho is implicitly defined by: observer -> w -> infinity
    //    At each vertex do:
    //    - test the visibility of the vertex
    //    - remove from tau any incident edge to the ccw of the sweep ray
    //    - add to tau any incident edge to the cw of the sweep ray
    //

    // The previously visited vertex in CW order.
    const Vertex *prev_w = nullptr;
    for (Vertex *w : vertices_cw)
    {
        util::Log(logDEBUG) << "Now at node id:" << w->ref() << " with " << tau.size()
                            << " edges in tau";

        // test visibility p -> w
        for (const Segment &s : tau)
        {
            util::Log(logDEBUG) << "    Edge id:" << s.first->ref() << " -> id:" << s.second->ref();
        }
        w->visible = visible(&observer, prev_w, w, tau);
        prev_w = w;
        util::Log(logDEBUG) << "    Result: " << w->visible;

        // update tau
        //
        // Delete the old edges to the CCW (left) of the sweep ray, then insert the new
        // edges to the CW (right) of it.  An edge incident to w that leads "to the
        // right" is crossed by the ray from here until we reach its far endpoint, so it
        // can go into tau straight away -- we no longer have to defer the insertion
        // until the ray properly crosses it, which is what the removed pending-edge
        // queue was for.  Note this runs *after* the visibility test for w, and that
        // intersect() ignores endpoint hits, so an edge incident to w never hides w.
        auto update_tau = [&](const Vertex *u, const Vertex *v, bool left)
        {
            if (left)
            {
                util::Log(logDEBUG) << "  Erasing edge: id:" << u->ref() << " -> id:" << v->ref();
                std::erase_if(tau,
                              [u, v](const Segment &s)
                              { return (*s.first == *u) && (*s.second == *v); });
            }
            else
            {
                tau.emplace_back(u, v);
                util::Log(logDEBUG)
                    << "  Edge inserted into tau: id:" << u->ref() << " -> id:" << v->ref();
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
    // An edge that shares an endpoint with the segment we are testing meets it only in
    // that endpoint -- two straight segments that share an endpoint can only meet again
    // if they are collinear, and intersect() rejects the collinear case as parallel --
    // so such an edge can never obstruct anything and has to be skipped.
    //
    // intersect() is meant to ignore endpoint hits by itself, but it cannot be relied on
    // to do so: it locates the hit by solving for the segment parameters, and for a
    // shared endpoint the solution comes out at 0.999999999999957 rather than exactly 1.
    // The endpoint is then mistaken for a proper crossing and a perfectly good line of
    // sight disappears.  tau legitimately holds edges incident to w (an edge entering
    // tau at its first endpoint stays there until the sweep reaches the second one), so
    // this is reached routinely, not just in contrived geometry.
    const auto same_point = [](const Vertex *a, const Vertex *b)
    { return a->point[0] == b->point[0] && a->point[1] == b->point[1]; };
    const auto touches = [&](const Segment &s, const Vertex *p)
    { return same_point(s.first, p) || same_point(s.second, p); };

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
        // If observer -> w intersects any edge in tau, then w is not visible.
        for (const Segment &s : tau)
        {
            if (touches(s, observer) || touches(s, w))
                continue;
            if (intersect(observer, w, s.first, s.second))
            {
                util::Log(logDEBUG)
                    << "    Invisible because obstructed by edge id:" << s.first->ref()
                    << " -> id:" << s.second->ref();
                return false;
            }
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
        if (touches(s, prev_w) || touches(s, w))
            continue;
        if (intersect(prev_w, w, s.first, s.second))
            return false;
    }
    return true;
}

} // namespace osrm::extractor::area
