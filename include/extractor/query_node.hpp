#ifndef QUERY_NODE_HPP
#define QUERY_NODE_HPP

#include "util/typedefs.hpp"

#include "util/coordinate.hpp"

#include <limits>

namespace osrm
{
namespace extractor
{

struct QueryNode
{
    using key_type = OSMNodeID; // type of NodeID
    using value_type = int;     // type of lat,lons

    explicit QueryNode(const util::FixedLongitude lon_,
                       const util::FixedLatitude lat_,
                       key_type node_id)
        : lon(lon_), lat(lat_), node_id(std::move(node_id))
    {
    }
    QueryNode()
        : lon(std::numeric_limits<int>::max()), lat(std::numeric_limits<int>::max()),
          node_id(SPECIAL_OSM_NODEID)
    {
    }

    util::FixedLongitude lon;
    util::FixedLatitude lat;
    key_type node_id;

    static QueryNode min_value()
    {
        return QueryNode(util::FixedLongitude(-180 * COORDINATE_PRECISION),
                         util::FixedLatitude(-90 * COORDINATE_PRECISION),
                         MIN_OSM_NODEID);
    }

    static QueryNode max_value()
    {
        return QueryNode(util::FixedLongitude(180 * COORDINATE_PRECISION),
                         util::FixedLatitude(90 * COORDINATE_PRECISION),
                         MAX_OSM_NODEID);
    }
};
}
}

#endif // QUERY_NODE_HPP
