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

    static _Node min_value()
    {
        return _Node(0,0,0);
    }
    static _Node max_value()
    {
        return _Node(numeric_limits<int>::max(), numeric_limits<int>::max(), numeric_limits<unsigned int>::max());
    }
    NodeID key() const
    {
        return id;
    }
};

struct _Coordinate {
    int lat;
    int lon;
    _Coordinate () : lat(INT_MIN), lon(INT_MIN) {}
    _Coordinate (int t, int n) : lat(t) , lon(n) {}
};

struct _Way {
    _Way() {
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

struct _Relation {
    enum {
        unknown = 0, ferry
    } type;
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
};

struct Settings {
    struct SpeedProfile {
        vector< double > speed;
        vector< string > names;
    } speedProfile;
    //    vector<string> accessList;
    //    int trafficLightPenalty;
    int indexInAccessListOf( const string & key)
    {
        for(unsigned i = 0; i< speedProfile.names.size(); i++)
        {
            if(speedProfile.names[i] == key)
                return i;
        }
        return -1;
    }
};

struct Cmp : public std::binary_function<NodeID, NodeID, bool>
{
    typedef NodeID value_type;
    bool operator ()  (const NodeID & a, const NodeID & b) const
    {
        return a < b;
    }
    value_type max_value()
    {
        return 0xffffffff;
    }
    value_type min_value()
    {
        return 0x0;
    }
};

struct CompareEdgeByStart : public std::binary_function<_Edge, _Edge, bool>
{
    typedef _Edge value_type;
    bool operator ()  (const _Edge & a, const _Edge & b) const
    {
        return a.start < b.start;
    }
    value_type max_value()
    {
        return _Edge(UINT_MAX, UINT_MAX);
    }
    value_type min_value()
    {
        return _Edge(0, 0);
    }
};

struct CompareEdgeByTarget : public std::binary_function<_Edge, _Edge, bool>
{
    typedef _Edge value_type;
    bool operator ()  (const _Edge & a, const _Edge & b) const
    {
        return a.target < b.target;
    }
    value_type max_value()
    {
        return _Edge(UINT_MAX, UINT_MAX);
    }
    value_type min_value()
    {
        return _Edge(0, 0);
    }
};

struct CmpNodeByID : public std::binary_function<_Node, _Node, bool>
{
    typedef _Node value_type;
    bool operator ()  (const _Node & a, const _Node & b) const
    {
        return a.id < b.id;
    }
    value_type max_value()
    {
        return _Node::max_value();
    }
    value_type min_value()
    {
        return _Node::min_value();
    }
};

double ApproximateDistance( const int lat1, const int lon1, const int lat2, const int lon2 ) {
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
double GetAngleBetweenTwoEdges(const _Coordinate& A, const _Coordinate& C, const _Coordinate& B)
{
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

typedef google::dense_hash_map<NodeID, _Node> NodeMap;

#endif /* EXTRACTORSTRUCTS_H_ */
