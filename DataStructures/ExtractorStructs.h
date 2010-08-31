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
#include <libxml/xmlreader.h>

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
        unclassified    50
        residential     40
        living_street   10
        service         30
        ferry           25
 */

std::string names[14] = { "motorway", "motorway_link", "trunk", "trunk_link", "primary", "primary_link", "secondary", "secondary_link", "tertiary", "unclassified", "residential", "living_street", "service", "ferry" };
double speeds[14] = { 110, 90, 90, 70, 70, 60, 60, 50, 55, 50, 40 , 10, 30, 25};

struct _Node : NodeInfo{
    bool trafficSignal;

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
    std::vector< NodeID > path;
    enum {
        notSure = 0, oneway, bidirectional, opposite
    } direction;
    double maximumSpeed;
    bool usefull:1;
    bool access:1;
    short type;
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

    _Coordinate startCoord;
    _Coordinate targetCoord;
};

struct Settings {
    struct SpeedProfile {
        vector< double > speed;
        vector< string > names;
    } speedProfile;
    vector<string> accessList;
    int trafficLightPenalty;
    int indexInAccessListOf( const string & key)
    {
        for(int i = 0; i< accessList.size(); i++)
        {
            if(accessList[i] == key)
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

_Way _ReadXMLWay( xmlTextReaderPtr& inputReader, Settings& settings ) {
    _Way way;
    way.direction = _Way::notSure;
    way.maximumSpeed = -1;
    way.type = -1;
    way.usefull = false;
    way.access = true;

    if ( xmlTextReaderIsEmptyElement( inputReader ) != 1 ) {
        const int depth = xmlTextReaderDepth( inputReader );
        while ( xmlTextReaderRead( inputReader ) == 1 ) {
            const int childType = xmlTextReaderNodeType( inputReader );
            if ( childType != 1 && childType != 15 )
                continue;
            const int childDepth = xmlTextReaderDepth( inputReader );
            xmlChar* childName = xmlTextReaderName( inputReader );
            if ( childName == NULL )
                continue;

            if ( depth == childDepth && childType == 15 && xmlStrEqual( childName, ( const xmlChar* ) "way" ) == 1 ) {
                xmlFree( childName );
                break;
            }
            if ( childType != 1 ) {
                xmlFree( childName );
                continue;
            }

            if ( xmlStrEqual( childName, ( const xmlChar* ) "tag" ) == 1 ) {
                xmlChar* k = xmlTextReaderGetAttribute( inputReader, ( const xmlChar* ) "k" );
                xmlChar* value = xmlTextReaderGetAttribute( inputReader, ( const xmlChar* ) "v" );
                if ( k != NULL && value != NULL ) {
                    if ( xmlStrEqual( k, ( const xmlChar* ) "oneway" ) == 1 ) {
                        if ( xmlStrEqual( value, ( const xmlChar* ) "no" ) == 1 || xmlStrEqual( value, ( const xmlChar* ) "false" ) == 1 || xmlStrEqual( value, ( const xmlChar* ) "0" ) == 1 )
                            way.direction = _Way::bidirectional;
                        else if ( xmlStrEqual( value, ( const xmlChar* ) "yes" ) == 1 || xmlStrEqual( value, ( const xmlChar* ) "true" ) == 1 || xmlStrEqual( value, ( const xmlChar* ) "1" ) == 1 )
                            way.direction = _Way::oneway;
                        else if ( xmlStrEqual( value, ( const xmlChar* ) "-1" ) == 1 )
                            way.direction = _Way::opposite;
                    } else if ( xmlStrEqual( k, ( const xmlChar* ) "junction" ) == 1 ) {
                        if ( xmlStrEqual( value, ( const xmlChar* ) "roundabout" ) == 1 ) {
                            if ( way.direction == _Way::notSure ) {
                                way.direction = _Way::oneway;
                            }
                            if ( way.maximumSpeed == -1 )
                                way.maximumSpeed = 10;
                            way.usefull = true;
                        }
                    } else if ( xmlStrEqual( k, ( const xmlChar* ) "route" ) == 1 ) {
                        string route( (const char* ) value );
                        if (route == "ferry") {
                            for ( int i = 0; i < settings.speedProfile.names.size(); i++ ) {
                                way.maximumSpeed = 5;
                                way.usefull = true;
                                way.direction == _Way::oneway;
                            }
                        }
                    } else if ( xmlStrEqual( k, ( const xmlChar* ) "highway" ) == 1 ) {
                        string name( ( const char* ) value );
                        for ( int i = 0; i < settings.speedProfile.names.size(); i++ ) {
                            if ( name == settings.speedProfile.names[i] ) {
                                way.type = i;
                                way.usefull = true;
                                break;
                            }
                        }
                        if ( name == "motorway"  ) {
                            if ( way.direction == _Way::notSure ) {
                                way.direction = _Way::oneway;
                            }
                        } else if ( name == "motorway_link" ) {
                            if ( way.direction == _Way::notSure ) {
                                way.direction = _Way::oneway;
                            }
                        }
                    } else if ( xmlStrEqual( k, ( const xmlChar* ) "maxspeed" ) == 1 ) {
                        double maxspeed = atof(( const char* ) value );

                        xmlChar buffer[100];
                        xmlStrPrintf( buffer, 100, ( const xmlChar* ) "%.lf", maxspeed );
                        if ( xmlStrEqual( value, buffer ) == 1 ) {
                            way.maximumSpeed = maxspeed;
                        } else {
                            xmlStrPrintf( buffer, 100, ( const xmlChar* ) "%.lf kmh", maxspeed );
                            if ( xmlStrEqual( value, buffer ) == 1 ) {
                                way.maximumSpeed = maxspeed;
                            } else {
                                xmlStrPrintf( buffer, 100, ( const xmlChar* ) "%.lfkmh", maxspeed );
                                if ( xmlStrEqual( value, buffer ) == 1 ) {
                                    way.maximumSpeed = maxspeed;
                                } else {
                                    xmlStrPrintf( buffer, 100, ( const xmlChar* ) "%.lf km/h", maxspeed );
                                    if ( xmlStrEqual( value, buffer ) == 1 ) {
                                        way.maximumSpeed = maxspeed;
                                    } else {
                                        xmlStrPrintf( buffer, 100, ( const xmlChar* ) "%.lfkm/h", maxspeed );
                                        if ( xmlStrEqual( value, buffer ) == 1 ) {
                                            way.maximumSpeed = maxspeed;
                                        } else {
                                            xmlStrPrintf( buffer, 100, ( const xmlChar* ) "%.lf mph", maxspeed );
                                            if ( xmlStrEqual( value, buffer ) == 1 ) {
                                                way.maximumSpeed = maxspeed;
                                            } else {
                                                xmlStrPrintf( buffer, 100, ( const xmlChar* ) "%.lfmph", maxspeed );
                                                if ( xmlStrEqual( value, buffer ) == 1 ) {
                                                    way.maximumSpeed = maxspeed;
                                                } else {
                                                    xmlStrPrintf( buffer, 100, ( const xmlChar* ) "%.lf mp/h", maxspeed );
                                                    if ( xmlStrEqual( value, buffer ) == 1 ) {
                                                        way.maximumSpeed = maxspeed;
                                                    } else {
                                                        xmlStrPrintf( buffer, 100, ( const xmlChar* ) "%.lfmp/h", maxspeed );
                                                        if ( xmlStrEqual( value, buffer ) == 1 ) {
                                                            way.maximumSpeed = maxspeed;
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    } else {
                        if ( xmlStrEqual( k, (const xmlChar*) "access" ))
                        {
                            if ( xmlStrEqual( value, ( const xmlChar* ) "private" ) == 1)

                                if ( xmlStrEqual( value, ( const xmlChar* ) "private" ) == 1
                                        || xmlStrEqual( value, ( const xmlChar* ) "no" ) == 1
                                        || xmlStrEqual( value, ( const xmlChar* ) "agricultural" ) == 1
                                        || xmlStrEqual( value, ( const xmlChar* ) "forestry" ) == 1
                                        || xmlStrEqual( value, ( const xmlChar* ) "delivery" ) == 1
                                ) {
                                    way.access = false;
                                }
                                else if ( xmlStrEqual( value, ( const xmlChar* ) "yes" ) == 1
                                        || xmlStrEqual( value, ( const xmlChar* ) "designated" ) == 1
                                        || xmlStrEqual( value, ( const xmlChar* ) "official" ) == 1
                                        || xmlStrEqual( value, ( const xmlChar* ) "permissive" ) == 1
                                ) {
                                    way.access = true;
                                }
                        }
                        if ( xmlStrEqual( k, (const xmlChar*) "motorcar" ))
                        {
                            if ( xmlStrEqual( value, ( const xmlChar* ) "yes" ) == 1)
                            {
                                way.access = true;
                            } else if ( xmlStrEqual( k, (const xmlChar*) "no" )) {
                                way.access = false;
                            }

                        }
                    }
                    if ( k != NULL )
                        xmlFree( k );
                    if ( value != NULL )
                        xmlFree( value );
                }
            } else if ( xmlStrEqual( childName, ( const xmlChar* ) "nd" ) == 1 ) {
                xmlChar* ref = xmlTextReaderGetAttribute( inputReader, ( const xmlChar* ) "ref" );
                if ( ref != NULL ) {
                    way.path.push_back( atoi(( const char* ) ref ) );
                    xmlFree( ref );
                }
            }
            xmlFree( childName );
        }
    }
    return way;
}

_Node _ReadXMLNode( xmlTextReaderPtr& inputReader ) {
    _Node node;
    node.trafficSignal = false;

    xmlChar* attribute = xmlTextReaderGetAttribute( inputReader, ( const xmlChar* ) "lat" );
    if ( attribute != NULL ) {
        node.lat =  static_cast<NodeID>(100000.*atof(( const char* ) attribute ) );
        xmlFree( attribute );
    }
    attribute = xmlTextReaderGetAttribute( inputReader, ( const xmlChar* ) "lon" );
    if ( attribute != NULL ) {
        node.lon =  static_cast<NodeID>(100000.*atof(( const char* ) attribute ));
        xmlFree( attribute );
    }
    attribute = xmlTextReaderGetAttribute( inputReader, ( const xmlChar* ) "id" );
    if ( attribute != NULL ) {
        node.id =  atoi(( const char* ) attribute );
        xmlFree( attribute );
    }

    if ( xmlTextReaderIsEmptyElement( inputReader ) != 1 ) {
        const int depth = xmlTextReaderDepth( inputReader );
        while ( xmlTextReaderRead( inputReader ) == 1 ) {
            const int childType = xmlTextReaderNodeType( inputReader );
            // 1 = Element, 15 = EndElement
            if ( childType != 1 && childType != 15 )
                continue;
            const int childDepth = xmlTextReaderDepth( inputReader );
            xmlChar* childName = xmlTextReaderName( inputReader );
            if ( childName == NULL )
                continue;

            if ( depth == childDepth && childType == 15 && xmlStrEqual( childName, ( const xmlChar* ) "node" ) == 1 ) {
                xmlFree( childName );
                break;
            }
            if ( childType != 1 ) {
                xmlFree( childName );
                continue;
            }

            if ( xmlStrEqual( childName, ( const xmlChar* ) "tag" ) == 1 ) {
                xmlChar* k = xmlTextReaderGetAttribute( inputReader, ( const xmlChar* ) "k" );
                xmlChar* value = xmlTextReaderGetAttribute( inputReader, ( const xmlChar* ) "v" );
                if ( k != NULL && value != NULL ) {
                    if ( xmlStrEqual( k, ( const xmlChar* ) "highway" ) == 1 ) {
                        if ( xmlStrEqual( value, ( const xmlChar* ) "traffic_signals" ) == 1 )
                            node.trafficSignal = true;
                    }
                }
                if ( k != NULL )
                    xmlFree( k );
                if ( value != NULL )
                    xmlFree( value );
            }

            xmlFree( childName );
        }
    }
    return node;
}

_Relation _ReadXMLRelation ( xmlTextReaderPtr& inputReader ) {
    _Relation relation;
    relation.type = _Relation::unknown;

    return relation;
}

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

#endif /* EXTRACTORSTRUCTS_H_ */
