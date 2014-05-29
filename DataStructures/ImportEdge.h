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
    bool operator<(const NodeBasedEdge &e) const;

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
                           bool is_split);

    NodeID target() const;
    NodeID source() const;
    NodeID name() const;
    EdgeWeight weight() const;
    short type() const;
    bool isBackward() const;
    bool isForward() const;
    bool isLocatable() const;
    bool isRoundabout() const;
    bool ignoreInGrid() const;
    bool isAccessRestricted() const;
    bool isContraFlow() const;
    bool IsSplit() const;

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

    NodeBasedEdge() = delete;
};

class EdgeBasedEdge
{

  public:
    bool operator<(const EdgeBasedEdge &e) const;

    template <class EdgeT>
    explicit EdgeBasedEdge(const EdgeT &myEdge);

    /** Default constructor. target and weight are set to 0.*/
    EdgeBasedEdge();

    explicit EdgeBasedEdge(const NodeID s,
                           const NodeID t,
                           const NodeID v,
                           const EdgeWeight w,
                           const bool f,
                           const bool b);

    NodeID target() const;
    NodeID source() const;
    EdgeWeight weight() const;
    NodeID id() const;
    bool isBackward() const;
    bool isForward() const;

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
