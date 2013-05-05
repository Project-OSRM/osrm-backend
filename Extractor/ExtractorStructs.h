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
#include "../DataStructures/TimingUtil.h"
#include "../DataStructures/TravelMode.h"
#include "../typedefs.h"

typedef boost::unordered_map<std::string, NodeID > StringMap;
typedef boost::unordered_map<std::string, std::pair<int, short> > StringToIntPairMap;


struct ExtractionWay {

    struct SettingsForDirection {
        SettingsForDirection() : speed(-1), mode(0) {}
        
    	double speed;
        TravelMode mode;
    };

    ExtractionWay() :
            id(UINT_MAX),
        	nameID(UINT_MAX),
            duration(-1),
            access(true),
            roundabout(false),
            isAccessRestricted(false),
            ignoreInGrid(false) {
    	path.clear();
    	keyVals.EraseAll();
    }
    
    enum Directions {
        oneway, bidirectional, opposite
    };
    
    inline bool HasDuration() { return duration>0; }
    inline bool IsBidirectional() { return forward.mode!=0 && backward.mode!=0; }
    inline bool IsOneway() { return forward.mode!=0 && backward.mode==0; }
    inline bool IsOpposite() { return forward.mode==0 && backward.mode!=0; }
    inline bool HasDiffDirections() { return (forward.mode != backward.mode) || (forward.speed != backward.speed); }
    inline Directions Direction() {
        if( IsOneway() ) {
            return ExtractionWay::oneway;
        }
        if( IsOpposite() ) {
            return ExtractionWay::opposite;
        }
        return ExtractionWay::bidirectional;
    }
    
    inline void set_mode(const TravelMode m) { forward.mode = m; backward.mode = m; }
    inline const TravelMode get_mode() {
        if( forward.mode == backward.mode ) {
            return forward.mode;
        } else {
            return -1;
        }
    }

    inline void set_speed(const double s) { forward.speed = s; backward.speed = s; }
    inline const double get_speed() {
        if( forward.speed == backward.speed ) {
            return forward.speed;
        } else {
            return -1;
        }
    }
    
    
    unsigned id;
    unsigned nameID;
    std::string name;
    double duration;
    bool access;
    bool roundabout;
    bool isAccessRestricted;
    bool ignoreInGrid;
    SettingsForDirection forward;
    SettingsForDirection backward;
    std::vector< NodeID > path;
    HashTable<std::string, std::string> keyVals;
};

struct ExtractorRelation {
    ExtractorRelation() : type(unknown){}
    enum {
        unknown = 0, ferry, turnRestriction
    } type;
    HashTable<std::string, std::string> keyVals;
};

struct InternalExtractorEdge {
    InternalExtractorEdge() : start(0), target(0), direction(0), speed(0), nameID(0), isRoundabout(false), ignoreInGrid(false), isDurationSet(false), isAccessRestricted(false), mode(0) {};
    InternalExtractorEdge(NodeID s, NodeID t) : start(s), target(t), direction(0), speed(0), nameID(0), isRoundabout(false), ignoreInGrid(false), isDurationSet(false), isAccessRestricted(false) {}
    InternalExtractorEdge(NodeID s, NodeID t, short d, double sp): start(s), target(t), direction(d), speed(sp), nameID(0), isRoundabout(false), ignoreInGrid(false), isDurationSet(false), isAccessRestricted(false) {}
    InternalExtractorEdge(NodeID s, NodeID t, short d, double sp, unsigned nid, bool isra, bool iing, bool ids, bool iar, TravelMode _mode): start(s), target(t), direction(d), speed(sp), nameID(nid), isRoundabout(isra), ignoreInGrid(iing), isDurationSet(ids), isAccessRestricted(iar), mode(_mode) {}
    NodeID start;
    NodeID target;
    short direction;
    double speed;
    unsigned nameID;
    bool isRoundabout;
    bool ignoreInGrid;
    bool isDurationSet;
    bool isAccessRestricted;
    TravelMode mode;
    
    _Coordinate startCoord;
    _Coordinate targetCoord;

    static InternalExtractorEdge min_value() {
        return InternalExtractorEdge(0,0);
    }
    static InternalExtractorEdge max_value() {
        return InternalExtractorEdge((std::numeric_limits<unsigned>::max)(), (std::numeric_limits<unsigned>::max)());
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
        return _WayIDStartAndEndEdge((std::numeric_limits<unsigned>::min)(), (std::numeric_limits<unsigned>::min)(), (std::numeric_limits<unsigned>::min)(), (std::numeric_limits<unsigned>::min)(), (std::numeric_limits<unsigned>::min)());
    }
    static _WayIDStartAndEndEdge max_value() {
        return _WayIDStartAndEndEdge((std::numeric_limits<unsigned>::max)(), (std::numeric_limits<unsigned>::max)(), (std::numeric_limits<unsigned>::max)(), (std::numeric_limits<unsigned>::max)(), (std::numeric_limits<unsigned>::max)());
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

struct CmpEdgeByStartID : public std::binary_function<InternalExtractorEdge, InternalExtractorEdge, bool> {
    typedef InternalExtractorEdge value_type;
    bool operator ()  (const InternalExtractorEdge & a, const InternalExtractorEdge & b) const {
        return a.start < b.start;
    }
    value_type max_value() {
        return InternalExtractorEdge::max_value();
    }
    value_type min_value() {
        return InternalExtractorEdge::min_value();
    }
};

struct CmpEdgeByTargetID : public std::binary_function<InternalExtractorEdge, InternalExtractorEdge, bool> {
    typedef InternalExtractorEdge value_type;
    bool operator ()  (const InternalExtractorEdge & a, const InternalExtractorEdge & b) const {
        return a.target < b.target;
    }
    value_type max_value() {
        return InternalExtractorEdge::max_value();
    }
    value_type min_value() {
        return InternalExtractorEdge::min_value();
    }
};

inline std::string GetRandomString() {
    char s[128];
    static const char alphanum[] =
            "0123456789"
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz";

    for (int i = 0; i < 127; ++i) {
        s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
    }
    s[127] = 0;
    return std::string(s);
}

#endif /* EXTRACTORSTRUCTS_H_ */
