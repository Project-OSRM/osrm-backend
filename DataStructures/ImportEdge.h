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

#ifndef IMPORT_EDGE_H
#define IMPORT_EDGE_H

#include "../Util/OSRMException.h"
#include "../typedefs.h"

#include <boost/assert.hpp>

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

    explicit NodeBasedEdge(
        NodeID s,
        NodeID t,
        NodeID n,
        EdgeWeight w,
        bool f,
        bool b,
        short ty,
        bool ra,
        bool ig,
        bool ar,
        bool cf
    ) : _source(s),
        _target(t),
        _name(n),
        _weight(w),
        _type(ty),
        forward(f),
        backward(b),
        _roundabout(ra),
        _ignoreInGrid(ig),
        _accessRestricted(ar),
        _contraFlow(cf)
    {
        if(ty < 0) {
            throw OSRMException("negative edge type");
        }
    }

    NodeID target()             const {return _target; }
    NodeID source()             const {return _source; }
    NodeID name()               const { return _name;  }
    EdgeWeight weight()         const {return _weight; }
    short type()                const {
        BOOST_ASSERT_MSG(_type >= 0, "type of ImportEdge invalid");
                                        return _type;   }
    bool isBackward()           const { return backward; }
    bool isForward()            const { return forward; }
    bool isLocatable()          const { return _type != 14; }
    bool isRoundabout()         const { return _roundabout; }
    bool ignoreInGrid()         const { return _ignoreInGrid; }
    bool isAccessRestricted()   const { return _accessRestricted; }
    bool isContraFlow()         const { return _contraFlow; }

    //TODO: names need to be fixed.
    NodeID      _source;
    NodeID      _target;
    NodeID      _name;
    EdgeWeight  _weight;
    short       _type;
    bool        forward:1;
    bool        backward:1;
    bool        _roundabout:1;
    bool        _ignoreInGrid:1;
    bool        _accessRestricted:1;
    bool        _contraFlow:1;

private:
    NodeBasedEdge() { }
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

    explicit EdgeBasedEdge(
        const NodeID s,
        const NodeID t,
        const NodeID v,
        const EdgeWeight w,
        const bool f,
        const bool b
    ) :
        m_source(s),
        m_target(t),
        m_edgeID(v),
        m_weight(w),
        m_forward(f),
        m_backward(b)
    {}

    NodeID      target()        const { return m_target;   }
    NodeID      source()        const { return m_source;   }
    EdgeWeight  weight()        const { return m_weight;   }
    NodeID      id()            const { return m_edgeID;   }
    bool        isBackward()    const { return m_backward; }
    bool        isForward()     const { return m_forward;  }

private:
    NodeID      m_source;
    NodeID      m_target;
    NodeID      m_edgeID;
    EdgeWeight  m_weight:30;
    bool        m_forward:1;
    bool        m_backward:1;
};

typedef NodeBasedEdge ImportEdge;

#endif /* IMPORT_EDGE_H */
