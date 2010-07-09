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

#ifndef EDGE_H
#define EDGE_H

#include <cassert>

class Edge
{
public:

    bool operator< (const Edge& e) const {
        if (source() == e.source()) {
            if (target() == e.target()) {
                if (weight() == e.weight()) {
                    return (isForward() && isBackward() &&
                            ((! e.isForward()) || (! e.isBackward())));
                }
                return (weight() < e.weight());
            }
            return (target() < e.target());
        }
        return (source() < e.source());
    }

    /** Default constructor. target and weight are set to 0.*/
    Edge() { assert(false); } //shall not be used.

    explicit Edge(NodeID s, NodeID t, EdgeWeight w, bool f, bool b) : _source(s), _target(t), _weight(w), forward(f), backward(b) { }

    NodeID target() const {return _target; }
    NodeID source() const {return _source;}
    EdgeWeight weight() const {return _weight; }

    bool isBackward() const { return backward; }

    bool isForward() const { return forward; }

private:
    NodeID _source;
    NodeID _target;
    EdgeWeight _weight:30;
    bool forward:1;
    bool backward:1;
};

typedef Edge ImportEdge;

#endif // EDGE_H
