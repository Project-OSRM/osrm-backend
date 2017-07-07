#ifndef FIRST_AND_LAST_SEGMENT_OF_WAY_HPP
#define FIRST_AND_LAST_SEGMENT_OF_WAY_HPP

#include "util/typedefs.hpp"

#include <limits>
#include <string>

namespace osrm
{
namespace extractor
{

struct FirstAndLastSegmentOfWay
{
    OSMWayID way_id;
    OSMNodeID first_segment_source_id;
    OSMNodeID first_segment_target_id;
    OSMNodeID last_segment_source_id;
    OSMNodeID last_segment_target_id;

    FirstAndLastSegmentOfWay()
        : way_id(SPECIAL_OSM_WAYID), first_segment_source_id(SPECIAL_OSM_NODEID),
          first_segment_target_id(SPECIAL_OSM_NODEID), last_segment_source_id(SPECIAL_OSM_NODEID),
          last_segment_target_id(SPECIAL_OSM_NODEID)
    {
    }

    FirstAndLastSegmentOfWay(OSMWayID w, OSMNodeID fs, OSMNodeID ft, OSMNodeID ls, OSMNodeID lt)
        : way_id(std::move(w)), first_segment_source_id(std::move(fs)),
          first_segment_target_id(std::move(ft)), last_segment_source_id(std::move(ls)),
          last_segment_target_id(std::move(lt))
    {
    }

    static FirstAndLastSegmentOfWay min_value()
    {
        return {MIN_OSM_WAYID, MIN_OSM_NODEID, MIN_OSM_NODEID, MIN_OSM_NODEID, MIN_OSM_NODEID};
    }
    static FirstAndLastSegmentOfWay max_value()
    {
        return {MAX_OSM_WAYID, MAX_OSM_NODEID, MAX_OSM_NODEID, MAX_OSM_NODEID, MAX_OSM_NODEID};
    }
};

struct FirstAndLastSegmentOfWayCompare
{
    using value_type = FirstAndLastSegmentOfWay;
    bool operator()(const FirstAndLastSegmentOfWay &a, const FirstAndLastSegmentOfWay &b) const
    {
        return a.way_id < b.way_id;
    }
    value_type max_value() { return FirstAndLastSegmentOfWay::max_value(); }
    value_type min_value() { return FirstAndLastSegmentOfWay::min_value(); }
};
}
}

#endif /* FIRST_AND_LAST_SEGMENT_OF_WAY_HPP */
