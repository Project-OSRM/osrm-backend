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

struct _GridEdge {
    _GridEdge(NodeID s, NodeID t, _Coordinate sc, _Coordinate tc) : start(s), target(t), startCoord(sc), targetCoord(tc) {}
    _GridEdge() : start(UINT_MAX), target(UINT_MAX) {}
    NodeID start;
    NodeID target;
    _Coordinate startCoord;
    _Coordinate targetCoord;
};

struct GridEntry {
    GridEntry() : fileIndex(UINT_MAX), ramIndex(UINT_MAX){}
    GridEntry(_GridEdge e, unsigned f, unsigned r) : edge(e), fileIndex(f), ramIndex(r) {}
    _GridEdge edge;
    unsigned fileIndex;
    unsigned ramIndex;
    bool operator< ( const GridEntry& right ) const {
        if(right.edge.start != edge.start)
            return right.edge.start < edge.start;
        if(right.edge.target != edge.target)
            return right.edge.target < edge.target;
        return false;
    }
    bool operator==( const GridEntry& right ) const {
        return right.edge.start == edge.start && right.edge.target == edge.target;
    }
};

struct CompareGridEdgeDataByFileIndex
{
    bool operator ()  (const GridEntry & a, const GridEntry & b) const
    {
        return a.fileIndex < b.fileIndex;
    }
};

struct CompareGridEdgeDataByRamIndex
{
    typedef GridEntry value_type;

    bool operator ()  (const GridEntry & a, const GridEntry & b) const
    {
        return a.ramIndex < b.ramIndex;
    }
    value_type max_value()
    {
        GridEntry e;
        e.ramIndex = (1024*1024) - 1;
        return e;
    }
    value_type min_value()
    {
        GridEntry e;
        e.ramIndex = 0;
        return e;
    }
};

#endif /* GRIDEDGE_H_ */
