/*

Copyright (c) 2014, Project OSRM contributors
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list
of conditions and the following disclaimer.
Redistributions in binary form must reproduce the above copyright notice, this
list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#ifndef INTERNAL_EXTRACTOR_EDGE_HPP
#define INTERNAL_EXTRACTOR_EDGE_HPP

#include "../typedefs.h"
#include "../data_structures/travel_mode.hpp"
#include "../data_structures/import_edge.hpp"

#include <boost/assert.hpp>

#include <osrm/coordinate.hpp>
#include <utility>

struct InternalExtractorEdge
{
    // specify the type of the weight data
    enum class WeightType : char {
        INVALID,
        SPEED,
        EDGE_DURATION,
        WAY_DURATION,
    };

    struct WeightData
    {

        WeightData() : duration(0.0), type(WeightType::INVALID)
        {
        }

        union
        {
            double duration;
            double speed;
        };
        WeightType type;
    };

    explicit InternalExtractorEdge()
        : result(MIN_OSM_NODEID, MIN_OSM_NODEID, 0, 0, false, false, false, false, true,
                TRAVEL_MODE_INACCESSIBLE, false)
    {
    }

    explicit InternalExtractorEdge(OSMNodeID source,
                                   OSMNodeID target,
                                   NodeID name_id,
                                   WeightData weight_data,
                                   bool forward,
                                   bool backward,
                                   bool roundabout,
                                   bool access_restricted,
                                   bool startpoint,
                                   TravelMode travel_mode,
                                   bool is_split)
        : result(OSMNodeID(source),
                 OSMNodeID(target),
                 name_id,
                 0,
                 forward,
                 backward,
                 roundabout,
                 access_restricted,
                 startpoint,
                 travel_mode,
                 is_split),
          weight_data(std::move(weight_data))
    {
    }

    // data that will be written to disk
    NodeBasedEdgeWithOSM result;
    // intermediate edge weight
    WeightData weight_data;
    // coordinate of the source node
    FixedPointCoordinate source_coordinate;


    // necessary static util functions for stxxl's sorting
    static InternalExtractorEdge min_osm_value()
    {
        return InternalExtractorEdge(MIN_OSM_NODEID, MIN_OSM_NODEID, 0, WeightData(), false, false, false,
                                     false, true, TRAVEL_MODE_INACCESSIBLE, false);
    }
    static InternalExtractorEdge max_osm_value()
    {
        return InternalExtractorEdge(MAX_OSM_NODEID, MAX_OSM_NODEID, 0, WeightData(), false,
                                     false, false, false, true, TRAVEL_MODE_INACCESSIBLE, false);
    }

    static InternalExtractorEdge min_internal_value()
    {
        auto v = min_osm_value();
        v.result.source = 0;
        v.result.target = 0;
        return v;
    }
    static InternalExtractorEdge max_internal_value()
    {
        auto v = max_osm_value();
        v.result.source = std::numeric_limits<NodeID>::max();
        v.result.target = std::numeric_limits<NodeID>::max();
        return v;
    }

};

struct CmpEdgeByInternalStartThenInternalTargetID
{
    using value_type = InternalExtractorEdge;
    bool operator()(const InternalExtractorEdge &lhs, const InternalExtractorEdge &rhs) const
    {
        return (lhs.result.source <  rhs.result.source) ||
              ((lhs.result.source == rhs.result.source) &&
               (lhs.result.target <  rhs.result.target));
    }

    value_type max_value() { return InternalExtractorEdge::max_internal_value(); }
    value_type min_value() { return InternalExtractorEdge::min_internal_value(); }
};

struct CmpEdgeByOSMStartID
{
    using value_type = InternalExtractorEdge;
    bool operator()(const InternalExtractorEdge &lhs, const InternalExtractorEdge &rhs) const
    {
        return lhs.result.osm_source_id < rhs.result.osm_source_id;
    }

    value_type max_value() { return InternalExtractorEdge::max_osm_value(); }
    value_type min_value() { return InternalExtractorEdge::min_osm_value(); }
};

struct CmpEdgeByOSMTargetID
{
    using value_type = InternalExtractorEdge;
    bool operator()(const InternalExtractorEdge &lhs, const InternalExtractorEdge &rhs) const
    {
        return lhs.result.osm_target_id < rhs.result.osm_target_id;
    }

    value_type max_value() { return InternalExtractorEdge::max_osm_value(); }
    value_type min_value() { return InternalExtractorEdge::min_osm_value(); }
};

#endif // INTERNAL_EXTRACTOR_EDGE_HPP
