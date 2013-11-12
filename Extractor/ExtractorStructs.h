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

#ifndef EXTRACTORSTRUCTS_H_
#define EXTRACTORSTRUCTS_H_

#include "../DataStructures/Coordinate.h"
#include "../DataStructures/HashTable.h"
#include "../DataStructures/ImportNode.h"
#include "../DataStructures/QueryNode.h"
#include "../DataStructures/Restriction.h"
#include "../typedefs.h"

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/regex.hpp>
#include <boost/regex.hpp>
#include <boost/unordered_map.hpp>

#include <climits>
#include <string>

typedef boost::unordered_map<std::string, NodeID > StringMap;
typedef boost::unordered_map<std::string, std::pair<int, short> > StringToIntPairMap;

struct ExtractionWay {
    ExtractionWay() {
		Clear();
    }

	inline void Clear(){
		id = UINT_MAX;
		nameID = UINT_MAX;
		path.clear();
		keyVals.clear();
        direction = ExtractionWay::notSure;
        speed = -1;
        backward_speed = -1;
        duration = -1;
        type = -1;
        access = true;
        roundabout = false;
        isAccessRestricted = false;
        ignoreInGrid = false;
    }

    enum Directions {
        notSure = 0, oneway, bidirectional, opposite
    };
    Directions direction;
    unsigned id;
    unsigned nameID;
    std::string name;
    double speed;
    double backward_speed;
    double duration;
    short type;
    bool access;
    bool roundabout;
    bool isAccessRestricted;
    bool ignoreInGrid;
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
    InternalExtractorEdge() : start(0), target(0), type(0), direction(0), speed(0), nameID(0), isRoundabout(false), ignoreInGrid(false), isDurationSet(false), isAccessRestricted(false), isContraFlow(false) {};
    InternalExtractorEdge(NodeID s, NodeID t) : start(s), target(t), type(0), direction(0), speed(0), nameID(0), isRoundabout(false), ignoreInGrid(false), isDurationSet(false), isAccessRestricted(false), isContraFlow(false) { }
    InternalExtractorEdge(NodeID s, NodeID t, short tp, short d, double sp): start(s), target(t), type(tp), direction(d), speed(sp), nameID(0), isRoundabout(false), ignoreInGrid(false), isDurationSet(false), isAccessRestricted(false), isContraFlow(false) { }
    InternalExtractorEdge(NodeID s, NodeID t, short tp, short d, double sp, unsigned nid, bool isra, bool iing, bool ids, bool iar): start(s), target(t), type(tp), direction(d), speed(sp), nameID(nid), isRoundabout(isra), ignoreInGrid(iing), isDurationSet(ids), isAccessRestricted(iar), isContraFlow(false) {
        assert(0 <= type);
    }
    InternalExtractorEdge(NodeID s, NodeID t, short tp, short d, double sp, unsigned nid, bool isra, bool iing, bool ids, bool iar, bool icf): start(s), target(t), type(tp), direction(d), speed(sp), nameID(nid), isRoundabout(isra), ignoreInGrid(iing), isDurationSet(ids), isAccessRestricted(iar), isContraFlow(icf) {
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
    bool isContraFlow;

    FixedPointCoordinate startCoord;
    FixedPointCoordinate targetCoord;

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
    bool operator ()  (const NodeID a, const NodeID b) const {
        return a < b;
    }
    value_type max_value() {
        return 0xffffffff;
    }
    value_type min_value() {
        return 0x0;
    }
};

struct CmpNodeByID : public std::binary_function<ExternalMemoryNode, ExternalMemoryNode, bool> {
    typedef ExternalMemoryNode value_type;
    bool operator () (
        const ExternalMemoryNode & a,
        const ExternalMemoryNode & b
    ) const {
        return a.id < b.id;
    }
    value_type max_value()  {
        return ExternalMemoryNode::max_value();
    }
    value_type min_value() {
        return ExternalMemoryNode::min_value();
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
