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
    if (source == e.source)
    {
        if (target == e.target)
        {
            if (weight == e.weight)
            {
                return (forward && backward && ((!e.forward) || (!e.backward)));
            }
            return (weight < e.weight);
        }
        return (target < e.target);
    }
    return (source < e.source);
}

NodeBasedEdge::NodeBasedEdge(NodeID source,
                             NodeID target,
                             NodeID name_id,
                             EdgeWeight weight,
                             bool forward,
                             bool backward,
                             short type,
                             bool roundabout,
                             bool in_tiny_cc,
                             bool access_restricted,
                             bool contra_flow,
                             bool is_split)
    : source(source), target(target), name_id(name_id), weight(weight), type(type),
      forward(forward), backward(backward), roundabout(roundabout), in_tiny_cc(in_tiny_cc),
      access_restricted(access_restricted), contra_flow(contra_flow), is_split(is_split)
{
    BOOST_ASSERT_MSG(type > 0, "negative edge type");
}

bool EdgeBasedEdge::operator<(const EdgeBasedEdge &e) const
{
    if (source == e.source)
    {
        if (target == e.target)
        {
            if (weight == e.weight)
            {
                return (forward && backward && ((!e.forward) || (!e.backward)));
            }
            return (weight < e.weight);
        }
        return (target < e.target);
    }
    return (source < e.source);
}

template <class EdgeT>
EdgeBasedEdge::EdgeBasedEdge(const EdgeT &myEdge)
    : source(myEdge.source), target(myEdge.target), edge_id(myEdge.data.via),
      weight(myEdge.data.distance), forward(myEdge.data.forward),
      backward(myEdge.data.backward)
{
}

/** Default constructor. target and weight are set to 0.*/
EdgeBasedEdge::EdgeBasedEdge()
    : source(0), target(0), edge_id(0), weight(0), forward(false), backward(false)
{
}

EdgeBasedEdge::EdgeBasedEdge(
    const NodeID s, const NodeID t, const NodeID v, const EdgeWeight w, const bool f, const bool b)
    : source(s), target(t), edge_id(v), weight(w), forward(f), backward(b)
{
}
