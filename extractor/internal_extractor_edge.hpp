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
        : result(0, 0, 0, 0, false, false, false, false, false,
                TRAVEL_MODE_INACCESSIBLE, false)
    {
    }

    explicit InternalExtractorEdge(NodeID source,
                           NodeID target,
                           NodeID name_id,
                           const WeightData& weight_data,
                           bool forward,
                           bool backward,
                           bool roundabout,
                           bool in_tiny_cc,
                           bool access_restricted,
                           TravelMode travel_mode,
                           bool is_split)
        : result(source, target, name_id, 0, forward, backward, roundabout,
                 in_tiny_cc, access_restricted, travel_mode, is_split),
          weight_data(weight_data)
    {
    }

    // data that will be written to disk
    NodeBasedEdge result;
    // intermediate edge weight
    WeightData weight_data;
    // coordinate of the source node
    FixedPointCoordinate source_coordinate;


    // necessary static util functions for stxxl's sorting
    static InternalExtractorEdge min_value()
    {
        return InternalExtractorEdge(0, 0, 0, WeightData(), false, false, false,
                                     false, false, TRAVEL_MODE_INACCESSIBLE, false);
    }
    static InternalExtractorEdge max_value()
    {
        return InternalExtractorEdge(SPECIAL_NODEID, SPECIAL_NODEID, 0, WeightData(), false, false,
                                     false, false, false, TRAVEL_MODE_INACCESSIBLE, false);
    }
};

struct CmpEdgeByStartThenTargetID
{
    using value_type = InternalExtractorEdge;
    bool operator()(const InternalExtractorEdge &lhs, const InternalExtractorEdge &rhs) const
    {
        return (lhs.result.source <  rhs.result.source) ||
              ((lhs.result.source == rhs.result.source) &&
               (lhs.result.target <  rhs.result.target));
    }

    value_type max_value() { return InternalExtractorEdge::max_value(); }
    value_type min_value() { return InternalExtractorEdge::min_value(); }
};

struct CmpEdgeByStartID
{
    using value_type = InternalExtractorEdge;
    bool operator()(const InternalExtractorEdge &lhs, const InternalExtractorEdge &rhs) const
    {
        return lhs.result.source < rhs.result.source;
    }

    value_type max_value() { return InternalExtractorEdge::max_value(); }
    value_type min_value() { return InternalExtractorEdge::min_value(); }
};

struct CmpEdgeByTargetID
{
    using value_type = InternalExtractorEdge;
    bool operator()(const InternalExtractorEdge &lhs, const InternalExtractorEdge &rhs) const
    {
        return lhs.result.target < rhs.result.target;
    }

    value_type max_value() { return InternalExtractorEdge::max_value(); }
    value_type min_value() { return InternalExtractorEdge::min_value(); }
};

#endif // INTERNAL_EXTRACTOR_EDGE_HPP
