#ifndef QUERY_NODE_HPP
#define QUERY_NODE_HPP

#include "util/typedefs.hpp"

#include "util/coordinate.hpp"

#include <cstdint>
#include <limits>

namespace osrm
{
namespace extractor
{

struct QueryNode
{
    using key_type = OSMNodeID;      // type of NodeID
    using value_type = std::int32_t; // type of lat,lons

    explicit QueryNode(const util::FixedLongitude lon_,
                       const util::FixedLatitude lat_,
                       const key_type node_id_)
        : lon(lon_), lat(lat_), node_id(node_id_)
    {
    }
    QueryNode()
        : lon{std::numeric_limits<value_type>::max()}, lat{std::numeric_limits<value_type>::max()},
          node_id(SPECIAL_OSM_NODEID)
    {
    }

    util::FixedLongitude lon;
    util::FixedLatitude lat;
    key_type node_id;

    static QueryNode min_value()
    {
        return QueryNode(util::FixedLongitude{static_cast<value_type>(-180 * COORDINATE_PRECISION)},
                         util::FixedLatitude{static_cast<value_type>(-90 * COORDINATE_PRECISION)},
                         MIN_OSM_NODEID);
    }

    static QueryNode max_value()
    {
        return QueryNode(util::FixedLongitude{static_cast<value_type>(180 * COORDINATE_PRECISION)},
                         util::FixedLatitude{static_cast<value_type>(90 * COORDINATE_PRECISION)},
                         MAX_OSM_NODEID);
    }
};
} // namespace extractor
} // namespace osrm

#endif // QUERY_NODE_HPP
