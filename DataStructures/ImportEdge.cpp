/*

Copyright (c) 2014, Project OSRM, Dennis Luxen, others
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

#include "ImportEdge.h"

bool NodeBasedEdge::operator<(const NodeBasedEdge &e) const
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

    NodeBasedEdge::NodeBasedEdge(NodeID s,
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

    NodeID NodeBasedEdge::target() const { return _target; }
    NodeID NodeBasedEdge::source() const { return _source; }
    NodeID NodeBasedEdge::name() const { return _name; }
    EdgeWeight NodeBasedEdge::weight() const { return _weight; }
    short NodeBasedEdge::type() const
    {
        BOOST_ASSERT_MSG(_type >= 0, "type of ImportEdge invalid");
        return _type;
    }
    bool NodeBasedEdge::isBackward() const { return backward; }
    bool NodeBasedEdge::isForward() const { return forward; }
    bool NodeBasedEdge::isLocatable() const { return _type != 14; }
    bool NodeBasedEdge::isRoundabout() const { return _roundabout; }
    bool NodeBasedEdge::ignoreInGrid() const { return _ignoreInGrid; }
    bool NodeBasedEdge::isAccessRestricted() const { return _accessRestricted; }
    bool NodeBasedEdge::isContraFlow() const { return _contraFlow; }
    bool NodeBasedEdge::IsSplit() const { return is_split; }

    bool EdgeBasedEdge::operator<(const EdgeBasedEdge &e) const
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
    EdgeBasedEdge::EdgeBasedEdge(const EdgeT &myEdge)
        : m_source(myEdge.source), m_target(myEdge.target), m_edgeID(myEdge.data.via),
          m_weight(myEdge.data.distance), m_forward(myEdge.data.forward),
          m_backward(myEdge.data.backward)
    {
    }

    /** Default constructor. target and weight are set to 0.*/
    EdgeBasedEdge::EdgeBasedEdge()
        : m_source(0), m_target(0), m_edgeID(0), m_weight(0), m_forward(false), m_backward(false)
    {
    }

    EdgeBasedEdge::EdgeBasedEdge(const NodeID s,
                           const NodeID t,
                           const NodeID v,
                           const EdgeWeight w,
                           const bool f,
                           const bool b)
        : m_source(s), m_target(t), m_edgeID(v), m_weight(w), m_forward(f), m_backward(b)
    {
    }

    NodeID EdgeBasedEdge::target() const { return m_target; }
    NodeID EdgeBasedEdge::source() const { return m_source; }
    EdgeWeight EdgeBasedEdge::weight() const { return m_weight; }
    NodeID EdgeBasedEdge::id() const { return m_edgeID; }
    bool EdgeBasedEdge::isBackward() const { return m_backward; }
    bool EdgeBasedEdge::isForward() const { return m_forward; }
