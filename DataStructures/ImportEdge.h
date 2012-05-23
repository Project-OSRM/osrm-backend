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

class NodeBasedEdge {
public:

    bool operator< (const NodeBasedEdge& e) const {
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
    NodeBasedEdge() :
        _source(0), _target(0), _name(0), _weight(0), forward(0), backward(0), _type(0), _roundabout(false), _ignoreInGrid(false), _accessRestricted(false) { assert(false); } //shall not be used.

    explicit NodeBasedEdge(NodeID s, NodeID t, NodeID n, EdgeWeight w, bool f, bool b, short ty, bool ra, bool ig, bool ar) :
            _source(s), _target(t), _name(n), _weight(w), forward(f), backward(b), _type(ty), _roundabout(ra), _ignoreInGrid(ig), _accessRestricted(ar) { if(ty < 0) {ERR("Type: " << ty);}; }

    NodeID target() const {return _target; }
    NodeID source() const {return _source; }
    NodeID name() const { return _name; }
    EdgeWeight weight() const {return _weight; }

    short type() const { assert(_type >= 0); return _type; }
    bool isBackward() const { return backward; }
    bool isForward() const { return forward; }
    bool isLocatable() const { return _type != 14; }
    bool isRoundabout() const { return _roundabout; }
    bool ignoreInGrid() const { return _ignoreInGrid; }
    bool isAccessRestricted() const { return _accessRestricted; }

    NodeID _source;
    NodeID _target;
    NodeID _name;
    EdgeWeight _weight;
    bool forward;
    bool backward;
    short _type;
    bool _roundabout;
    bool _ignoreInGrid;
    bool _accessRestricted;
};

class EdgeBasedEdge {
public:

    bool operator< (const EdgeBasedEdge& e) const {
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

    template<class EdgeT>
    EdgeBasedEdge(const EdgeT & myEdge ) :
        _source(myEdge.source),
        _target(myEdge.target),
        _edgeID(myEdge.data.via),
//        _nameID1(myEdge.data.nameID),
        _weight(myEdge.data.distance),
        _forward(myEdge.data.forward),
        _backward(myEdge.data.backward)//,
//        _turnInstruction(myEdge.data.turnInstruction)
                { }

    /** Default constructor. target and weight are set to 0.*/
    EdgeBasedEdge() :
        _source(0), _target(0), _edgeID(0), _weight(0), _forward(false), _backward(false) { }

    explicit EdgeBasedEdge(NodeID s, NodeID t, NodeID v, EdgeWeight w, bool f, bool b) :
            _source(s), _target(t), _edgeID(v), _weight(w), _forward(f), _backward(b){}

    NodeID target() const {return _target; }
    NodeID source() const {return _source; }
    EdgeWeight weight() const {return _weight; }
    NodeID id() const { return _edgeID; }
    bool isBackward() const { return _backward; }
    bool isForward() const { return _forward; }

    NodeID _source;
    NodeID _target;
    NodeID _edgeID;
    EdgeWeight _weight:30;
    bool _forward:1;
    bool _backward:1;
};

struct MinimalEdgeData {
public:
    EdgeWeight distance;
    bool forward;
    bool backward;
};

typedef NodeBasedEdge ImportEdge;

#endif // EDGE_H
