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

#ifndef IMPORT_EDGE_H
#define IMPORT_EDGE_H

#include "../Util/OSRMException.h"
#include "../typedefs.h"

#include <boost/assert.hpp>

class NodeBasedEdge
{

  public:
    bool operator<(const NodeBasedEdge &e) const
    {
        if (source() == e.source())
        {
            if (target() == e.target())
            {
                if (weight() == e.weight())
                {
                    return (isForward() && isBackward() && ((!e.isForward()) || (!e.isBackward())));
                }
                return (weight() < e.weight());
            }
            return (target() < e.target());
        }
        return (source() < e.source());
    }

    explicit NodeBasedEdge(NodeID s,
                           NodeID t,
                           NodeID n,
                           EdgeWeight w,
                           bool f,
                           bool b,
                           short ty,
                           bool ra,
                           bool ig,
                           bool ar,
                           bool cf,
                           bool is_split)
        : _source(s), _target(t), _name(n), _weight(w), _type(ty), forward(f), backward(b),
          _roundabout(ra), _ignoreInGrid(ig), _accessRestricted(ar), _contraFlow(cf),
          is_split(is_split)
    {
        if (ty < 0)
        {
            throw OSRMException("negative edge type");
        }
    }

    NodeID target() const { return _target; }
    NodeID source() const { return _source; }
    NodeID name() const { return _name; }
    EdgeWeight weight() const { return _weight; }
    short type() const
    {
        BOOST_ASSERT_MSG(_type >= 0, "type of ImportEdge invalid");
        return _type;
    }
    bool isBackward() const { return backward; }
    bool isForward() const { return forward; }
    bool isLocatable() const { return _type != 14; }
    bool isRoundabout() const { return _roundabout; }
    bool ignoreInGrid() const { return _ignoreInGrid; }
    bool isAccessRestricted() const { return _accessRestricted; }
    bool isContraFlow() const { return _contraFlow; }
    bool IsSplit() const { return is_split; }

    // TODO: names need to be fixed.
    NodeID _source;
    NodeID _target;
    NodeID _name;
    EdgeWeight _weight;
    short _type;
    bool forward : 1;
    bool backward : 1;
    bool _roundabout : 1;
    bool _ignoreInGrid : 1;
    bool _accessRestricted : 1;
    bool _contraFlow : 1;
    bool is_split : 1;

  private:
    NodeBasedEdge() {}
};

class EdgeBasedEdge
{

  public:
    bool operator<(const EdgeBasedEdge &e) const
    {
        if (source() == e.source())
        {
            if (target() == e.target())
            {
                if (weight() == e.weight())
                {
                    return (isForward() && isBackward() && ((!e.isForward()) || (!e.isBackward())));
                }
                return (weight() < e.weight());
            }
            return (target() < e.target());
        }
        return (source() < e.source());
    }

    template <class EdgeT>
    explicit EdgeBasedEdge(const EdgeT &myEdge)
        : m_source(myEdge.source), m_target(myEdge.target), m_edgeID(myEdge.data.via),
          m_weight(myEdge.data.distance), m_forward(myEdge.data.forward),
          m_backward(myEdge.data.backward)
    {
    }

    /** Default constructor. target and weight are set to 0.*/
    EdgeBasedEdge()
        : m_source(0), m_target(0), m_edgeID(0), m_weight(0), m_forward(false), m_backward(false)
    {
    }

    explicit EdgeBasedEdge(const NodeID s,
                           const NodeID t,
                           const NodeID v,
                           const EdgeWeight w,
                           const bool f,
                           const bool b)
        : m_source(s), m_target(t), m_edgeID(v), m_weight(w), m_forward(f), m_backward(b)
    {
    }

    NodeID target() const { return m_target; }
    NodeID source() const { return m_source; }
    EdgeWeight weight() const { return m_weight; }
    NodeID id() const { return m_edgeID; }
    bool isBackward() const { return m_backward; }
    bool isForward() const { return m_forward; }

  private:
    NodeID m_source;
    NodeID m_target;
    NodeID m_edgeID;
    EdgeWeight m_weight : 30;
    bool m_forward : 1;
    bool m_backward : 1;
};

typedef NodeBasedEdge ImportEdge;

#endif /* IMPORT_EDGE_H */
