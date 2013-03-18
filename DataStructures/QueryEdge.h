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



#ifndef QUERYEDGE_H_
#define QUERYEDGE_H_

#include "TurnInstructions.h"
#include "TravelMode.h"
#include "../typedefs.h"

#include <climits>

struct OriginalEdgeData{
    explicit OriginalEdgeData(NodeID v, unsigned n, TurnInstruction t, TravelMode _mode) : viaNode(v), nameID(n), turnInstruction(t), mode(_mode) {}
    OriginalEdgeData() : viaNode(UINT_MAX), nameID(UINT_MAX), turnInstruction(UCHAR_MAX) {}
    NodeID viaNode;
    unsigned nameID;
    TurnInstruction turnInstruction;
    TravelMode mode;
};

struct QueryEdge {
    NodeID source;
    NodeID target;
    struct EdgeData {
        EdgeData() : 
            id(0),
            shortcut(false),
            distance(0),
            forward(false),
            backward(false)
            {}

        NodeID id:31;
        bool shortcut:1;
        int distance:30;
        bool forward:1;
        bool backward:1;
    } data;
    bool operator<( const QueryEdge& right ) const {
        if ( source != right.source )
            return source < right.source;
        return target < right.target;
    }

    //sorts by source and other attributes
    static bool CompareBySource( const QueryEdge& left, const QueryEdge& right ) {
        if ( left.source != right.source )
            return left.source < right.source;
        int l = ( left.data.forward ? -1 : 0 ) + ( left.data.backward ? -1 : 0 );
        int r = ( right.data.forward ? -1 : 0 ) + ( right.data.backward ? -1 : 0 );
        if ( l != r )
            return l < r;
        if ( left.target != right.target )
            return left.target < right.target;
        return left.data.distance < right.data.distance;
    }

    bool operator== ( const QueryEdge& right ) const {
        return ( source == right.source && target == right.target && data.distance == right.data.distance &&
                data.shortcut == right.data.shortcut && data.forward == right.data.forward && data.backward == right.data.backward
                && data.id == right.data.id
        );
    }
};

#endif /* QUERYEDGE_H_ */
