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

struct _RawRestrictionContainer {
    TurnRestriction restriction;
    EdgeID fromWay;
    EdgeID toWay;
    unsigned viaNode;

    _RawRestrictionContainer(
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
    _RawRestrictionContainer(
        bool isOnly = false
    ) :
        fromWay(UINT_MAX),
        toWay(UINT_MAX),
        viaNode(UINT_MAX)
    {
        restriction.flags.isOnly = isOnly;
    }

    static _RawRestrictionContainer min_value() {
        return _RawRestrictionContainer(0, 0, 0, 0);
    }
    static _RawRestrictionContainer max_value() {
        return _RawRestrictionContainer(UINT_MAX, UINT_MAX, UINT_MAX, UINT_MAX);
    }
};

struct CmpRestrictionContainerByFrom : public std::binary_function<_RawRestrictionContainer, _RawRestrictionContainer, bool> {
    typedef _RawRestrictionContainer value_type;
    bool operator ()  (const _RawRestrictionContainer & a, const _RawRestrictionContainer & b) const {
        return a.fromWay < b.fromWay;
    }
    value_type max_value()  {
        return _RawRestrictionContainer::max_value();
    }
    value_type min_value() {
        return _RawRestrictionContainer::min_value();
    }
};

struct CmpRestrictionContainerByTo: public std::binary_function<_RawRestrictionContainer, _RawRestrictionContainer, bool> {
    typedef _RawRestrictionContainer value_type;
    bool operator ()  (const _RawRestrictionContainer & a, const _RawRestrictionContainer & b) const {
        return a.toWay < b.toWay;
    }
    value_type max_value()  {
        return _RawRestrictionContainer::max_value();
    }
    value_type min_value() {
        return _RawRestrictionContainer::min_value();
    }
};

#endif /* RESTRICTION_H_ */
