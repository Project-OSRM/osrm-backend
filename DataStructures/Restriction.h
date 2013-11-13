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

#ifndef RESTRICTION_H_
#define RESTRICTION_H_

#include "../typedefs.h"
#include <climits>

struct TurnRestriction {
    NodeID viaNode;
    NodeID fromNode;
    NodeID toNode;
    struct Bits { //mostly unused
        Bits()
         :
            isOnly(false),
            unused1(false),
            unused2(false),
            unused3(false),
            unused4(false),
            unused5(false),
            unused6(false),
            unused7(false)
        { }

        bool isOnly:1;
        bool unused1:1;
        bool unused2:1;
        bool unused3:1;
        bool unused4:1;
        bool unused5:1;
        bool unused6:1;
        bool unused7:1;
    } flags;

    TurnRestriction(NodeID viaNode) :
        viaNode(viaNode),
        fromNode(UINT_MAX),
        toNode(UINT_MAX) {

    }

    TurnRestriction(const bool isOnly = false) :
        viaNode(UINT_MAX),
        fromNode(UINT_MAX),
        toNode(UINT_MAX) {
        flags.isOnly = isOnly;
    }
};

struct InputRestrictionContainer {
    EdgeID fromWay;
    EdgeID toWay;
    unsigned viaNode;
    TurnRestriction restriction;

    InputRestrictionContainer(
        EdgeID fromWay,
        EdgeID toWay,
        NodeID vn,
        unsigned vw
    ) :
        fromWay(fromWay),
        toWay(toWay),
        viaNode(vw)
    {
        restriction.viaNode = vn;
    }
    InputRestrictionContainer(
        bool isOnly = false
    ) :
        fromWay(UINT_MAX),
        toWay(UINT_MAX),
        viaNode(UINT_MAX)
    {
        restriction.flags.isOnly = isOnly;
    }

    static InputRestrictionContainer min_value() {
        return InputRestrictionContainer(0, 0, 0, 0);
    }
    static InputRestrictionContainer max_value() {
        return InputRestrictionContainer(UINT_MAX, UINT_MAX, UINT_MAX, UINT_MAX);
    }
};

struct CmpRestrictionContainerByFrom :
    public std::binary_function<InputRestrictionContainer, InputRestrictionContainer, bool>
{
    typedef InputRestrictionContainer value_type;
    inline bool operator()(
        const InputRestrictionContainer & a,
        const InputRestrictionContainer & b
    ) const {
        return a.fromWay < b.fromWay;
    }
    inline value_type max_value() const {
        return InputRestrictionContainer::max_value();
    }
    inline value_type min_value() const {
        return InputRestrictionContainer::min_value();
    }
};

struct CmpRestrictionContainerByTo: public std::binary_function<InputRestrictionContainer, InputRestrictionContainer, bool> {
    typedef InputRestrictionContainer value_type;
    inline bool operator ()(
        const InputRestrictionContainer & a,
        const InputRestrictionContainer & b
    ) const {
        return a.toWay < b.toWay;
    }
    value_type max_value() const {
        return InputRestrictionContainer::max_value();
    }
    value_type min_value() const {
        return InputRestrictionContainer::min_value();
    }
};

#endif /* RESTRICTION_H_ */
