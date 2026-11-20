#ifndef OSRM_EXTRACTOR_NODE_LOCATION_INDEX_HPP
#define OSRM_EXTRACTOR_NODE_LOCATION_INDEX_HPP

#include "extractor/query_node.hpp"

#include <osmium/index/map/flex_mem.hpp>
#include <osmium/osm/location.hpp>
#include <osmium/osm/types.hpp>

namespace osrm::extractor
{
using NodeLocationIndex =
    osmium::index::map::FlexMem<osmium::unsigned_object_id_type, osmium::Location>;

inline QueryNode makeQueryNode(OSMNodeID id, osmium::Location location)
{
    return QueryNode{util::toFixed(util::UnsafeFloatLongitude{location.lon()}),
                     util::toFixed(util::UnsafeFloatLatitude{location.lat()}),
                     id};
}
} // namespace osrm::extractor

#endif
