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

#ifndef IMPORTNODE_H_
#define IMPORTNODE_H_

#include "QueryNode.h"
#include "../DataStructures/HashTable.h"


struct _Node : NodeInfo{
    _Node(int _lat, int _lon, unsigned int _id, bool _bollard, bool _trafficLight) : NodeInfo(_lat, _lon,  _id), bollard(_bollard), trafficLight(_trafficLight) {}
    _Node() : bollard(false), trafficLight(false) {}

    static _Node min_value() {
        return _Node(0,0,0, false, false);
    }
    static _Node max_value() {
        return _Node((std::numeric_limits<int>::max)(), (std::numeric_limits<int>::max)(), (std::numeric_limits<unsigned int>::max)(), false, false);
    }
    NodeID key() const {
        return id;
    }
    bool bollard;
    bool trafficLight;
};

struct ImportNode : public _Node {
    HashTable<std::string, std::string> keyVals;

	inline void Clear() {
		keyVals.clear();
		lat = 0; lon = 0; id = 0; bollard = false; trafficLight = false;
	}
};

#endif /* IMPORTNODE_H_ */
