/*
    open source routing machine
    Copyright (C) Dennis Luxen, others 2010

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU AFFERO General Public License as published by
the Free Software Foundation; either version 3 of the License, or
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
or see http://www.gnu.org/licenses/agpl.txt.
 */

#ifndef _NODE_COORDS_H
#define _NODE_COORDS_H

#include "Coordinate.h"
#include "../typedefs.h"

#include <boost/assert.hpp>

#include <cstddef>
#include <climits>

#include <limits>

struct NodeInfo {
	typedef NodeID key_type; 	//type of NodeID
	typedef int value_type;		//type of lat,lons

	NodeInfo(int _lat, int _lon, NodeID _id) : lat(_lat), lon(_lon), id(_id) {}
	NodeInfo() : lat(INT_MAX), lon(INT_MAX), id(UINT_MAX) {}
	int lat;
	int lon;
	NodeID id;

	static NodeInfo min_value() {
		return NodeInfo(
			 -90*COORDINATE_PRECISION,
			-180*COORDINATE_PRECISION,
			std::numeric_limits<NodeID>::min()
		);
	}

	static NodeInfo max_value() {
		return NodeInfo(
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
};

#endif //_NODE_COORDS_H
