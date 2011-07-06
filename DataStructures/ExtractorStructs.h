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
#include <cmath>
#include <string>
#include "HashTable.h"
#include "Util.h"

struct _PathData {
    _PathData(NodeID n) : node(n) { }
    NodeID node;
};

/*     Default Speed Profile:
        motorway        110
        motorway_link   90
        trunk           90
        trunk_link      70
        primary         70
        primary_link    60
        secondary       60
        secondary_link  50
        tertiary        55
        unclassified    25
        residential     40
        living_street   10
        service         30
        ferry           5
 */

typedef google::dense_hash_map<std::string, NodeID> StringMap;

std::string names[14] = { "motorway", "motorway_link", "trunk", "trunk_link", "primary", "primary_link", "secondary", "secondary_link", "tertiary", "unclassified", "residential", "living_street", "service", "ferry" };
double speeds[14] = { 110, 90, 90, 70, 70, 60, 60, 50, 55, 25, 40 , 10, 30, 5};

struct _Node : NodeInfo{
    _Node(int _lat, int _lon, unsigned int _id) : NodeInfo(_lat, _lon,  _id) {}
    _Node() {}

    static _Node min_value() {
        return _Node(0,0,0);
    }
    static _Node max_value() {
        return _Node(numeric_limits<int>::max(), numeric_limits<int>::max(), numeric_limits<unsigned int>::max());
    }
    NodeID key() const {
        return id;
    }
};

struct _Coordinate {
    int lat;
    int lon;
    _Coordinate () : lat(INT_MIN), lon(INT_MIN) {}
    _Coordinate (int t, int n) : lat(t) , lon(n) {}
};

ostream & operator<<(ostream & out, const _Coordinate & c){
    out << "(" << c.lat << "," << c.lon << ")";
    return out;
}

struct _Way {
    _Way() : id(UINT_MAX) {

        direction = _Way::notSure;
        maximumSpeed = -1;
        type = -1;
        useful = false;
        access = true;
    }

    std::vector< NodeID > path;
    enum {
        notSure = 0, oneway, bidirectional, opposite
    } direction;
    unsigned id;
    unsigned nameID;
    std::string name;
    double maximumSpeed;
    bool useful:1;
    bool access:1;
    short type;
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
    _Edge() {};
    _Edge(NodeID s, NodeID t) : start(s), target(t) { }
    _Edge(NodeID s, NodeID t, short tp, short d, double sp): start(s), target(t), type(tp), direction(d), speed(sp) { }
    NodeID start;
    NodeID target;
    short type;
    short direction;
    double speed;
    unsigned nameID;

    _Coordinate startCoord;
    _Coordinate targetCoord;

    static _Edge min_value() {
        return _Edge(0,0);
    }
    static _Edge max_value() {
        return _Edge(numeric_limits<unsigned>::max(), numeric_limits<unsigned>::max());
    }

};

struct _Restriction {
    NodeID viaNode;
    NodeID fromNode;
    NodeID toNode;
    struct bits { //mostly unused
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

struct _RawRestrictionContainer {
    _Restriction restriction;
    EdgeID fromWay;
    EdgeID toWay;
    unsigned viaWay;

    _RawRestrictionContainer(EdgeID f, EdgeID t, NodeID vn, unsigned vw) : fromWay(f), toWay(t), viaWay(vw) { restriction.viaNode = vn;}
    _RawRestrictionContainer(bool isOnly = false) : fromWay(UINT_MAX), toWay(UINT_MAX), viaWay(UINT_MAX) { restriction.flags.isOnly = isOnly;}

    static _RawRestrictionContainer min_value() {
        return _RawRestrictionContainer(numeric_limits<unsigned>::min(), numeric_limits<unsigned>::min(), numeric_limits<unsigned>::min(), numeric_limits<unsigned>::min());
    }
    static _RawRestrictionContainer max_value() {
        return _RawRestrictionContainer(numeric_limits<unsigned>::max(), numeric_limits<unsigned>::max(), numeric_limits<unsigned>::max(), numeric_limits<unsigned>::max());
    }
};


struct CmpRestrictionByFrom: public std::binary_function<_RawRestrictionContainer, _RawRestrictionContainer, bool> {
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

struct CmpRestrictionByTo: public std::binary_function<_RawRestrictionContainer, _RawRestrictionContainer, bool> {
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
        return _WayIDStartAndEndEdge(numeric_limits<unsigned>::min(), numeric_limits<unsigned>::min(), numeric_limits<unsigned>::min(), numeric_limits<unsigned>::min(), numeric_limits<unsigned>::min());
    }
    static _WayIDStartAndEndEdge max_value() {
        return _WayIDStartAndEndEdge(numeric_limits<unsigned>::max(), numeric_limits<unsigned>::max(), numeric_limits<unsigned>::max(), numeric_limits<unsigned>::max(), numeric_limits<unsigned>::max());
    }
};

struct CmpWayStartAndEnd : public std::binary_function<_WayIDStartAndEndEdge, _WayIDStartAndEndEdge, bool> {
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
    struct SpeedProfile {
        vector< double > speed;
        vector< string > names;
    } speedProfile;
    int indexInAccessListOf( const string & key) {
        for(unsigned i = 0; i< speedProfile.names.size(); i++) {
            if(speedProfile.names[i] == key)
                return i;
        }
        return -1;
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

double ApproximateDistance( const int lat1, const int lon1, const int lat2, const int lon2 ) {
    assert(lat1 != INT_MIN);
    assert(lon1 != INT_MIN);
    assert(lat2 != INT_MIN);
    assert(lon2 != INT_MIN);
    static const double DEG_TO_RAD = 0.017453292519943295769236907684886;
    ///Earth's quatratic mean radius for WGS-84
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

/* Get angle of line segment (A,C)->(C,B), atan2 magic, formerly cosine theorem*/
double GetAngleBetweenTwoEdges(const _Coordinate& A, const _Coordinate& C, const _Coordinate& B) {
    //    double a = ApproximateDistance(A.lat, A.lon, C.lat, C.lon); //first edge segment
    //    double b = ApproximateDistance(B.lat, B.lon, C.lat, C.lon); //second edge segment
    //    double c = ApproximateDistance(A.lat, A.lon, B.lat, B.lon); //third edgefrom triangle
    //
    //    double cosAlpha = (a*a + b*b - c*c)/ (2*a*b);
    //
    //    double alpha = ( (acos(cosAlpha) * 180.0 / M_PI) * (cosAlpha > 0 ? -1 : 1) ) + 180;
    //    return alpha;
    //    V = <x2 - x1, y2 - y1>
    int v1x = A.lon - C.lon;
    int v1y = A.lat - C.lat;
    int v2x = B.lon - C.lon;
    int v2y = B.lat - C.lat;

    double angle = (atan2(v2y,v2x) - atan2(v1y,v1x) )*180/M_PI;
    while(angle < 0)
        angle += 360;

    return angle;
}

string GetRandomString() {
    char s[128];
    static const char alphanum[] =
            "0123456789"
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz";

    for (int i = 0; i < 128; ++i) {
        s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
    }
    s[128] = 0;
    return string(s);
}

#endif /* EXTRACTORSTRUCTS_H_ */
