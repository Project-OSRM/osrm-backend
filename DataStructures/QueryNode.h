/*

Copyright (c) 2013, Project OSRM, Dennis Luxen, others
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

#ifndef _NODE_COORDS_H
#define _NODE_COORDS_H

#include "OSRM_config.h"

#include "../typedefs.h"

#include <osrm/Coordinate.h>

#include <boost/assert.hpp>

#include <cstddef>
#include <climits>

#include <limits>

struct NodeInfo2D {
	typedef NodeID key_type; 	//type of NodeID
	typedef int value_type;		//type of lat,lons

    NodeInfo2D(int _lat, int _lon, NodeID _id) : lat(_lat), lon(_lon), id(_id) {}
    NodeInfo2D() : lat(INT_MAX), lon(INT_MAX) {}

    int lat;
	int lon;
    NodeID id;

    static NodeInfo2D min_value() {
        return NodeInfo2D(
			 -90*COORDINATE_PRECISION,
			-180*COORDINATE_PRECISION,
			std::numeric_limits<NodeID>::min()
		);
	}

    static NodeInfo2D max_value() {
        return NodeInfo2D(
			 90*COORDINATE_PRECISION,
			180*COORDINATE_PRECISION,
			std::numeric_limits<NodeID>::max()
			);
	}

	value_type operator[](const std::size_t n) const {
		switch(n) {
        case 1:
			return lat;
			break;
		case 0:
			return lon;
			break;
		default:
			BOOST_ASSERT_MSG(false, "should not happen");
			return UINT_MAX;
			break;
		}
		BOOST_ASSERT_MSG(false, "should not happen");
		return UINT_MAX;
	}

    int get_ele() const { return std::numeric_limits<int>::max(); }
    void set_ele(const int /*e*/) {}
};

struct NodeInfo3D : public NodeInfo2D {
    typedef NodeInfo2D super;
    NodeInfo3D(int _lat, int _lon, NodeID _id) : super(_lat, _lon, _id), ele(std::numeric_limits<int>::max()){}
    NodeInfo3D(int _lat, int _lon, int _ele, NodeID _id) : super(_lat, _lon, _id), ele(_ele) {}
    NodeInfo3D() : super(), ele(std::numeric_limits<int>::max()) {}

    int ele;

    static NodeInfo3D min_value() {
        return NodeInfo3D(
             -90*COORDINATE_PRECISION,
            -180*COORDINATE_PRECISION,
            std::numeric_limits<int>::min(),
            std::numeric_limits<NodeID>::min()
        );
    }

    static NodeInfo3D max_value() {
        return NodeInfo3D(
             90*COORDINATE_PRECISION,
            180*COORDINATE_PRECISION,
            std::numeric_limits<int>::max(),
            std::numeric_limits<NodeID>::max()
            );
    }

    value_type operator[](const std::size_t n) const {
        switch(n) {
        case 2:
            return ele;
            break;
        default:
            return super::operator [](n);
            break;
        }
        BOOST_ASSERT_MSG(false, "should not happen");
        return UINT_MAX;
    }

    int get_ele() const { return ele; }
    void set_ele(const int e) { ele = e; }
};

#ifdef OSRM_HAS_ELEVATION
typedef NodeInfo3D NodeInfo;
#else
typedef NodeInfo2D NodeInfo;
#endif

#endif //_NODE_COORDS_H
