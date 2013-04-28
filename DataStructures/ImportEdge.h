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

#include "TravelMode.h"

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

    explicit NodeBasedEdge(NodeID s, NodeID t, NodeID n, EdgeWeight w, bool f, bool b, bool ra, bool ig, bool ar, TravelMode mode) :
                                            _source(s), _target(t), _name(n), _weight(w), forward(f), backward(b), _roundabout(ra), _ignoreInGrid(ig), _accessRestricted(ar), _mode(mode) {}

    NodeID target() const {return _target; }
    NodeID source() const {return _source; }
    NodeID name() const { return _name; }
    EdgeWeight weight() const {return _weight; }

    bool isBackward() const { return backward; }
    bool isForward() const { return forward; }
    bool isRoundabout() const { return _roundabout; }
    bool ignoreInGrid() const { return _ignoreInGrid; }
    bool isAccessRestricted() const { return _accessRestricted; }
    TravelMode mode() const { return _mode; }

    NodeID _source;
    NodeID _target;
    NodeID _name;
    EdgeWeight _weight;
    bool forward;
    bool backward;
    bool _roundabout;
    bool _ignoreInGrid;
    bool _accessRestricted;
    TravelMode    _mode;

private:
    /** Default constructor. target and weight are set to 0.*/
    NodeBasedEdge() :
        _source(0), _target(0), _name(0), _weight(0), forward(0), backward(0), _roundabout(false), _ignoreInGrid(false), _accessRestricted(false), _mode(0) { assert(false); } //shall not be used.

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
        m_source(myEdge.source),
        m_target(myEdge.target),
        m_edgeID(myEdge.data.via),
        m_weight(myEdge.data.distance),
        m_forward(myEdge.data.forward),
        m_backward(myEdge.data.backward)
    { }

    /** Default constructor. target and weight are set to 0.*/
    EdgeBasedEdge() :
        m_source(0),
        m_target(0),
        m_edgeID(0),
        m_weight(0),
        m_forward(false),
        m_backward(false)
    { }

    explicit EdgeBasedEdge(const NodeID s, const NodeID t, const NodeID v, const EdgeWeight w, const bool f, const bool b) :
        m_source(s),
        m_target(t),
        m_edgeID(v),
        m_weight(w),
        m_forward(f),
        m_backward(b)
    {}

    NodeID target() const {return m_target; }
    NodeID source() const {return m_source; }
    EdgeWeight weight() const {return m_weight; }
    NodeID id() const { return m_edgeID; }
    bool isBackward() const { return m_backward; }
    bool isForward() const { return m_forward; }  
private:
    NodeID m_source;
    NodeID m_target;
    NodeID m_edgeID;
    EdgeWeight m_weight:30;
    bool m_forward:1;
    bool m_backward:1;
};

typedef NodeBasedEdge ImportEdge;

#endif // EDGE_H
