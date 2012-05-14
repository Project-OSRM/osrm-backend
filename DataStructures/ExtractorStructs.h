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
#include <boost/unordered_map.hpp>
#include "../DataStructures/HashTable.h"
#include "../typedefs.h"
#include "Util.h"

struct _PathData {
    _PathData(NodeID no, unsigned na, unsigned tu, unsigned dur) : node(no), nameID(na), durationOfSegment(dur), turnInstruction(tu) { }
    NodeID node;
    unsigned nameID;
    unsigned durationOfSegment;
    short turnInstruction;
};

typedef boost::unordered_map<std::string, NodeID > StringMap;
typedef boost::unordered_map<std::string, std::pair<int, short> > StringToIntPairMap;

struct _Node : NodeInfo{
    _Node(int _lat, int _lon, unsigned int _id, bool _bollard, bool _trafficLight) : NodeInfo(_lat, _lon,  _id), bollard(_bollard), trafficLight(_trafficLight) {}
    _Node() : bollard(false), trafficLight(false) {}

    static _Node min_value() {
        return _Node(0,0,0, false, false);
    }
    static _Node max_value() {
        return _Node((std::numeric_limits<int>::max)(), (std::numeric_limits<int>::max)(), (std::numeric_limits<unsigned int>::max)(), false, false);
    }
    NodeID key() const {
        return id;
    }
    bool bollard;
    bool trafficLight;

};

struct _Coordinate {
    int lat;
    int lon;
    _Coordinate () : lat(INT_MIN), lon(INT_MIN) {}
    _Coordinate (int t, int n) : lat(t) , lon(n) {}
    void Reset() {
        lat = INT_MIN;
        lon = INT_MIN;
    }
    bool isSet() const {
        return (INT_MIN != lat) && (INT_MIN != lon);
    }
    inline bool isValid() const {
        if(lat > 90*100000 || lat < -90*100000 || lon > 180*100000 || lon <-180*100000) {
            return false;
        }
        return true;
    }

};

inline ostream & operator<<(ostream & out, const _Coordinate & c){
    out << "(" << c.lat << "," << c.lon << ")";
    return out;
}

struct _Way {
    _Way() : id(UINT_MAX), nameID(UINT_MAX) {

        direction = _Way::notSure;
        speed = -1;
        type = -1;
        useful = false;
        access = true;
        roundabout = false;
        isDurationSet = false;
        isAccessRestricted = false;
    }

    enum {
        notSure = 0, oneway, bidirectional, opposite
    } direction;
    unsigned id;
    unsigned nameID;
    std::string name;
    double speed;
    short type;
    bool useful;
    bool access;
    bool roundabout;
    bool isDurationSet;
    bool isAccessRestricted;
    std::vector< NodeID > path;
    HashTable<std::string, std::string> keyVals;
};

struct _Address {
    _Address() {}
    _Address(_Node n, std::string h, std::string str, std::string sta, std::string p, std::string ci, std::string co) {
        node = n;
        housenumber = h;
        street = str;
        state = sta;
        postcode = p;
        city = ci;
        country = co;
    }
    _Node node;
    std::string housenumber;
    std::string street;
    std::string state;
    std::string postcode;
    std::string city;
    std::string country;
};

struct _Relation {
    _Relation() : type(unknown){}
    enum {
        unknown = 0, ferry, turnRestriction
    } type;
    HashTable<std::string, std::string> keyVals;
};

struct _Edge {
    _Edge() : start(0), target(0), type(0), direction(0), speed(0), nameID(0), isRoundabout(false), ignoreInGrid(false), isDurationSet(false) {};
    _Edge(NodeID s, NodeID t) : start(s), target(t), type(0), direction(0), speed(0), nameID(0), isRoundabout(false), ignoreInGrid(false) { }
    _Edge(NodeID s, NodeID t, short tp, short d, double sp): start(s), target(t), type(tp), direction(d), speed(sp), nameID(0), isRoundabout(false), ignoreInGrid(false), isDurationSet(false) { }
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

struct _Restriction {
    NodeID viaNode;
    NodeID fromNode;
    NodeID toNode;
    struct Bits { //mostly unused
        Bits() : isOnly(false), unused1(false), unused2(false), unused3(false), unused4(false), unused5(false), unused6(false), unused7(false) {}
        char isOnly:1;
        char unused1:1;
        char unused2:1;
        char unused3:1;
        char unused4:1;
        char unused5:1;
        char unused6:1;
        char unused7:1;
    } flags;

    _Restriction(NodeID vn) : viaNode(vn), fromNode(UINT_MAX), toNode(UINT_MAX) { }
    _Restriction(bool isOnly = false) : viaNode(UINT_MAX), fromNode(UINT_MAX), toNode(UINT_MAX) {
        flags.isOnly = isOnly;
    }
};

inline bool CmpRestrictionByFrom ( _Restriction a, _Restriction b) { return (a.fromNode < b.fromNode);  }

struct _RawRestrictionContainer {
    _Restriction restriction;
    EdgeID fromWay;
    EdgeID toWay;
    unsigned viaNode;

    _RawRestrictionContainer(EdgeID f, EdgeID t, NodeID vn, unsigned vw) : fromWay(f), toWay(t), viaNode(vw) { restriction.viaNode = vn;}
    _RawRestrictionContainer(bool isOnly = false) : fromWay(UINT_MAX), toWay(UINT_MAX), viaNode(UINT_MAX) { restriction.flags.isOnly = isOnly;}

    static _RawRestrictionContainer min_value() {
        return _RawRestrictionContainer((numeric_limits<unsigned>::min)(), (numeric_limits<unsigned>::min)(), (numeric_limits<unsigned>::min)(), (numeric_limits<unsigned>::min)());
    }
    static _RawRestrictionContainer max_value() {
        return _RawRestrictionContainer((numeric_limits<unsigned>::max)(), (numeric_limits<unsigned>::max)(), (numeric_limits<unsigned>::max)(), (numeric_limits<unsigned>::max)());
    }
};

struct CmpRestrictionContainerByFrom: public std::binary_function<_RawRestrictionContainer, _RawRestrictionContainer, bool> {
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

struct Settings {
    Settings() : obeyBollards(true), obeyOneways(true), useRestrictions(true), ignoreAreas(false), accessTag("motorcar"), defaultSpeed(30), takeMinimumOfSpeeds(false), excludeFromGrid("ferry") {}
    StringToIntPairMap speedProfile;
    int operator[](const std::string & param) const {
        if(speedProfile.find(param) == speedProfile.end())
            return 0;
        else
            return speedProfile.at(param).first;
    }
    int GetHighwayTypeID(const std::string & param) const {
    	if(param == excludeFromGrid) {
    		return SHRT_MAX;
    	}
    	assert(param != "ferry");
        if(speedProfile.find(param) == speedProfile.end()) {
            ERR("There is a bug with highway \"" << param << "\"");
            return -1;
        } else {
            return speedProfile.at(param).second;
        }
    }
    bool obeyBollards;
    bool obeyOneways;
    bool useRestrictions;
    bool ignoreAreas;
    std::string accessTag;
    int defaultSpeed;
    bool takeMinimumOfSpeeds;
    std::string excludeFromGrid;
    boost::unordered_map<std::string, bool> accessRestrictedService;
    boost::unordered_map<std::string, bool> accessRestrictionKeys;
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

inline double ApproximateDistance( const int lat1, const int lon1, const int lat2, const int lon2 ) {
    assert(lat1 != INT_MIN);
    assert(lon1 != INT_MIN);
    assert(lat2 != INT_MIN);
    assert(lon2 != INT_MIN);
    static const double DEG_TO_RAD = 0.017453292519943295769236907684886;
    //Earth's quatratic mean radius for WGS-84
    static const double EARTH_RADIUS_IN_METERS = 6372797.560856;
    double latitudeArc  = ( lat1/100000. - lat2/100000. ) * DEG_TO_RAD;
    double longitudeArc = ( lon1/100000. - lon2/100000. ) * DEG_TO_RAD;
    double latitudeH = sin( latitudeArc * 0.5 );
    latitudeH *= latitudeH;
    double lontitudeH = sin( longitudeArc * 0.5 );
    lontitudeH *= lontitudeH;
    double tmp = cos( lat1/100000. * DEG_TO_RAD ) * cos( lat2/100000. * DEG_TO_RAD );
    double distanceArc =  2.0 * asin( sqrt( latitudeH + tmp * lontitudeH ) );
    return EARTH_RADIUS_IN_METERS * distanceArc;
}

inline double ApproximateDistance(const _Coordinate &c1, const _Coordinate &c2) {
    return ApproximateDistance( c1.lat, c1.lon, c2.lat, c2.lon );
}

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
