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

#ifndef GRIDEDGE_H_
#define GRIDEDGE_H_

#include "Coordinate.h"
#include "TravelMode.h"

struct _GridEdge {
    _GridEdge(NodeID n, NodeID na, int w, _Coordinate sc, _Coordinate tc, bool bttc, TravelMode _mode) : edgeBasedNode(n), nameID(na), weight(w), startCoord(sc), targetCoord(tc), belongsToTinyComponent(bttc), mode(_mode) {}
    _GridEdge() : edgeBasedNode(UINT_MAX), nameID(UINT_MAX), weight(INT_MAX), belongsToTinyComponent(false), mode(0) {}
    NodeID edgeBasedNode;
    NodeID nameID;
    int weight;
    _Coordinate startCoord;
    _Coordinate targetCoord;
    bool belongsToTinyComponent;
    TravelMode mode;

    bool operator< ( const _GridEdge& right) const {
        return edgeBasedNode < right.edgeBasedNode;
    }
    bool operator== ( const _GridEdge& right) const {
        return edgeBasedNode == right.edgeBasedNode;
    }
};

struct GridEntry {
    GridEntry() : fileIndex(UINT_MAX), ramIndex(UINT_MAX){}
    GridEntry(_GridEdge e, unsigned f, unsigned r) : edge(e), fileIndex(f), ramIndex(r) {}
    _GridEdge edge;
    unsigned fileIndex;
    unsigned ramIndex;
    bool operator< ( const GridEntry& right ) const {
        return (edge.edgeBasedNode < right.edge.edgeBasedNode);
    }
    bool operator==( const GridEntry& right ) const {
        return right.edge.edgeBasedNode == edge.edgeBasedNode;
    }
};

struct CompareGridEdgeDataByFileIndex {
    bool operator ()  (const GridEntry & a, const GridEntry & b) const {
        return a.fileIndex < b.fileIndex;
    }
};

struct CompareGridEdgeDataByRamIndex {
    typedef GridEntry value_type;

    bool operator ()  (const GridEntry & a, const GridEntry & b) const {
        return a.ramIndex < b.ramIndex;
    }
    value_type max_value() {
        GridEntry e;
        e.ramIndex = (1024*1024) - 1;
        return e;
    }
    value_type min_value() {
        GridEntry e;
        e.ramIndex = 0;
        return e;
    }
};

#endif /* GRIDEDGE_H_ */
