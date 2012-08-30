/*
    open source routing machine
    Copyright (C) Dennis Luxen, 2010

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

#ifndef EXTRACTORSTRUCTS_H_
#define EXTRACTORSTRUCTS_H_

#include <climits>
#include <string>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/regex.hpp>
#include <boost/regex.hpp>
#include <boost/unordered_map.hpp>

#include "../DataStructures/Coordinate.h"
#include "../DataStructures/HashTable.h"
#include "../DataStructures/ImportNode.h"
#include "../DataStructures/NodeCoords.h"
#include "../DataStructures/Restriction.h"
#include "../DataStructures/Util.h"
#include "../typedefs.h"

typedef boost::unordered_map<std::string, NodeID > StringMap;
typedef boost::unordered_map<std::string, std::pair<int, short> > StringToIntPairMap;

struct _Way {
    _Way() : id(UINT_MAX), nameID(UINT_MAX) {

        direction = _Way::notSure;
        speed = -1;
        type = -1;
//        useful = false;
        access = true;
        roundabout = false;
        isDurationSet = false;
        isAccessRestricted = false;
        ignoreInGrid = false;
    }

    enum {
        notSure = 0, oneway, bidirectional, opposite
    } direction;
    unsigned id;
    unsigned nameID;
    std::string name;
    double speed;
    short type;
    bool access;
    bool roundabout;
    bool isDurationSet;
    bool isAccessRestricted;
    bool ignoreInGrid;
    std::vector< NodeID > path;
    HashTable<std::string, std::string> keyVals;
};

struct _Relation {
    _Relation() : type(unknown){}
    enum {
        unknown = 0, ferry, turnRestriction
    } type;
    HashTable<std::string, std::string> keyVals;
};

struct _Edge {
    _Edge() : start(0), target(0), type(0), direction(0), speed(0), nameID(0), isRoundabout(false), ignoreInGrid(false), isDurationSet(false), isAccessRestricted(false) {};
    _Edge(NodeID s, NodeID t) : start(s), target(t), type(0), direction(0), speed(0), nameID(0), isRoundabout(false), ignoreInGrid(false), isDurationSet(false), isAccessRestricted(false) { }
    _Edge(NodeID s, NodeID t, short tp, short d, double sp): start(s), target(t), type(tp), direction(d), speed(sp), nameID(0), isRoundabout(false), ignoreInGrid(false), isDurationSet(false), isAccessRestricted(false) { }
    _Edge(NodeID s, NodeID t, short tp, short d, double sp, unsigned nid, bool isra, bool iing, bool ids, bool iar): start(s), target(t), type(tp), direction(d), speed(sp), nameID(nid), isRoundabout(isra), ignoreInGrid(iing), isDurationSet(ids), isAccessRestricted(iar) {
        assert(0 <= type);
    }
    NodeID start;
    NodeID target;
    short type;
    short direction;
    double speed;
    unsigned nameID;
    bool isRoundabout;
    bool ignoreInGrid;
    bool isDurationSet;
    bool isAccessRestricted;

    _Coordinate startCoord;
    _Coordinate targetCoord;

    static _Edge min_value() {
        return _Edge(0,0);
    }
    static _Edge max_value() {
        return _Edge((numeric_limits<unsigned>::max)(), (numeric_limits<unsigned>::max)());
    }

};

struct _WayIDStartAndEndEdge {
    unsigned wayID;
    NodeID firstStart;
    NodeID firstTarget;
    NodeID lastStart;
    NodeID lastTarget;
    _WayIDStartAndEndEdge() : wayID(UINT_MAX), firstStart(UINT_MAX), firstTarget(UINT_MAX), lastStart(UINT_MAX), lastTarget(UINT_MAX) {}
    _WayIDStartAndEndEdge(unsigned w, NodeID fs, NodeID ft, NodeID ls, NodeID lt) :  wayID(w), firstStart(fs), firstTarget(ft), lastStart(ls), lastTarget(lt) {}

    static _WayIDStartAndEndEdge min_value() {
        return _WayIDStartAndEndEdge((numeric_limits<unsigned>::min)(), (numeric_limits<unsigned>::min)(), (numeric_limits<unsigned>::min)(), (numeric_limits<unsigned>::min)(), (numeric_limits<unsigned>::min)());
    }
    static _WayIDStartAndEndEdge max_value() {
        return _WayIDStartAndEndEdge((numeric_limits<unsigned>::max)(), (numeric_limits<unsigned>::max)(), (numeric_limits<unsigned>::max)(), (numeric_limits<unsigned>::max)(), (numeric_limits<unsigned>::max)());
    }
};

struct CmpWayByID : public std::binary_function<_WayIDStartAndEndEdge, _WayIDStartAndEndEdge, bool> {
    typedef _WayIDStartAndEndEdge value_type;
    bool operator ()  (const _WayIDStartAndEndEdge & a, const _WayIDStartAndEndEdge & b) const {
        return a.wayID < b.wayID;
    }
    value_type max_value() {
        return _WayIDStartAndEndEdge::max_value();
    }
    value_type min_value() {
        return _WayIDStartAndEndEdge::min_value();
    }
};

struct Cmp : public std::binary_function<NodeID, NodeID, bool> {
    typedef NodeID value_type;
    bool operator ()  (const NodeID & a, const NodeID & b) const {
        return a < b;
    }
    value_type max_value() {
        return 0xffffffff;
    }
    value_type min_value() {
        return 0x0;
    }
};

struct CmpNodeByID : public std::binary_function<_Node, _Node, bool> {
    typedef _Node value_type;
    bool operator ()  (const _Node & a, const _Node & b) const {
        return a.id < b.id;
    }
    value_type max_value()  {
        return _Node::max_value();
    }
    value_type min_value() {
        return _Node::min_value();
    }
};

struct CmpEdgeByStartID : public std::binary_function<_Edge, _Edge, bool>
{
    typedef _Edge value_type;
    bool operator ()  (const _Edge & a, const _Edge & b) const {
        return a.start < b.start;
    }
    value_type max_value() {
        return _Edge::max_value();
    }
    value_type min_value() {
        return _Edge::min_value();
    }
};

struct CmpEdgeByTargetID : public std::binary_function<_Edge, _Edge, bool>
{
    typedef _Edge value_type;
    bool operator ()  (const _Edge & a, const _Edge & b) const
    {
        return a.target < b.target;
    }
    value_type max_value()
    {
        return _Edge::max_value();
    }
    value_type min_value()
    {
        return _Edge::min_value();
    }
};

inline string GetRandomString() {
    char s[128];
    static const char alphanum[] =
            "0123456789"
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz";

    for (int i = 0; i < 127; ++i) {
        s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
    }
    s[127] = 0;
    return string(s);
}

#endif /* EXTRACTORSTRUCTS_H_ */
