#ifndef OSRM_EXTRACTOR_AREA_TYPEDEFS_HPP
#define OSRM_EXTRACTOR_AREA_TYPEDEFS_HPP

#include <boost/geometry/core/cs.hpp>
#include <boost/geometry/geometries/multi_polygon.hpp>
#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/geometries/polygon.hpp>
#include <boost/geometry/srs/epsg.hpp>
#include <boost/geometry/srs/projection.hpp>

#include <osmium/osm/area.hpp>
#include <osmium/osm/node_ref.hpp>
#include <osmium/osm/types.hpp>
#include <set>

namespace osrm::extractor::area
{

namespace bg = boost::geometry;

using OsmiumPolygon = bg::model::polygon<osmium::NodeRef, false, false>;
using OsmiumMultiPolygon = bg::model::multi_polygon<OsmiumPolygon>;
using NodeRefSet = std::set<osmium::NodeRef>;

/**
 * @brief A segment between two nodes.
 */
struct OsmiumSegment : public std::pair<osmium::NodeRef, osmium::NodeRef>
{
    OsmiumSegment(const osmium::NodeRef &first, const osmium::NodeRef &second)
    {
        if (first.ref() < second.ref())
        {
            this->first = first;
            this->second = second;
        }
        else
        {
            this->first = second;
            this->second = first;
        }
    }
};

} // namespace osrm::extractor::area

//
// The following traits adapt the point-ish types we use to boost::geometry.
//

namespace boost
{

namespace geometry::traits
{

using namespace osrm::extractor::area;

// osmium::NodeRef

template <> struct tag<osmium::NodeRef>
{
    using type = point_tag; // osmium::NodeRef represents a point
};
template <> struct dimension<osmium::NodeRef> : boost::mpl::int_<2> // it has 2 dimensions
{
};
template <> struct coordinate_type<osmium::NodeRef>
{
    using type = double; // it actually stores the value multiplied by
                         // (double)osmium::detail::coordinate_precision in an int32_t
                         // but this is what we retrieve
};
template <> struct coordinate_system<osmium::NodeRef>
{
    using type = bg::cs::spherical_equatorial<bg::degree>; // it is spherical
};

template <> struct access<osmium::NodeRef, 0>
{
    static inline double get(const osmium::NodeRef &p) { return p.location().lon_without_check(); }
    static inline void set(osmium::NodeRef &p, double value) { p.location().set_lon(value); }
};

template <> struct access<osmium::NodeRef, 1>
{
    static inline double get(const osmium::NodeRef &p) { return p.location().lat_without_check(); }
    static inline void set(osmium::NodeRef &p, double value) { p.location().set_lat(value); }
};

// osmium::OuterRing

template <> struct tag<osmium::OuterRing>
{
    using type = ring_tag; // osmium::OuterRing represents a ring type
};
template <> struct point_order<osmium::OuterRing>
{
    static const order_selector value = counterclockwise;
};
template <> struct closure<osmium::OuterRing>
{
    static const closure_selector value = closed;
};

// osmium::InnerRing

template <> struct tag<osmium::InnerRing>
{
    using type = ring_tag; // osmium::InnerRing represents a ring type
};
template <> struct point_order<osmium::InnerRing>
{
    static const order_selector value = clockwise;
};
template <> struct closure<osmium::InnerRing>
{
    static const closure_selector value = closed;
};

} // namespace geometry::traits

template <> struct range_value<osmium::OuterRing>
{
    using type = osmium::NodeRef; // it is a ring of NodeRefs
};
template <> struct range_value<osmium::InnerRing>
{
    using type = osmium::NodeRef; // it is a ring of NodeRefs
};

} // namespace boost

#endif // OSRM_EXTRACTOR_AREA_TYPEDEFS_HPP
