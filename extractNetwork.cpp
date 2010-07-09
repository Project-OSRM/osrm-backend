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

#include <cassert>
#include <climits>
#include <cstdlib>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <libxml/xmlreader.h>

#include "typedefs.h"

using namespace std;

struct _Node : NodeInfo{
    bool trafficSignal:1;
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

typedef google::dense_hash_map<NodeID, _Node> NodeMap;

struct _Stats {
    NodeID numberOfNodes;
    NodeID numberOfEdges;
    NodeID numberOfWays;
    //    NodeID numberOfPlaces;
    //    NodeID numberOfOutlines;
    NodeID numberOfMaxspeed;
    //    NodeID numberOfZeroSpeed;
    //    NodeID numberOfDefaultCitySpeed;
    //    NodeID numberOfCityEdges;
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

_Way _ReadXMLWay( xmlTextReaderPtr& inputReader );
_Node _ReadXMLNode( xmlTextReaderPtr& inputReader );
double ApproximateDistance( const  int lat1, const  int lon1, const  int lat2, const  int lon2 );

_Stats stats;
Settings settings;
NodeMap AllNodes;

vector<NodeID> SignalNodes;
vector<NodeID> UsedNodes;
vector<_Way> UsedWays;

int main (int argc, char *argv[])
{
    if(argc <= 1)
    {
        cerr << "usage: " << endl << argv[0] << " <file.osm>" << endl;
        exit(-1);
    }
    cout << "reading input file. This may take some time ..." << flush;
    /*
     Default Speed Profile:
        motorway        120
        motorway_link   80
        trunk           100
        trunk_link      80
        secondary       100
        secondary_link  50
        primary         100
        primary_link    50
        tertiary        100
        unclassified    50
        residential     50
        living_street   30
        service         20
     */

    string names[13] = { "motorway", "motorway_link", "trunk", "trunk_link", "secondary", "secondary_link", "primary", "primary_link", "tertiary", "unclassified", "residential", "living_street", "service" };
    double speeds[13] = { 120, 80, 100, 80, 100, 50, 100, 50, 100, 50, 50 , 30, 20};
    settings.speedProfile.names.insert(settings.speedProfile.names.begin(), names, names+13);
    settings.speedProfile.speed.insert(settings.speedProfile.speed.begin(), speeds, speeds+13);

    AllNodes.set_empty_key(UINT_MAX);
    xmlTextReaderPtr inputReader = xmlNewTextReaderFilename( argv[1] );
    ofstream nodeFile("_nodes", ios::binary);
    ofstream wayFile("_ways", ios::binary);
    try {
        while ( xmlTextReaderRead( inputReader ) == 1 ) {
            const int type = xmlTextReaderNodeType( inputReader );

            //1 is Element
            if ( type != 1 )
                continue;

            xmlChar* currentName = xmlTextReaderName( inputReader );
            if ( currentName == NULL )
                continue;

            if ( xmlStrEqual( currentName, ( const xmlChar* ) "node" ) == 1 ) {
                stats.numberOfNodes++;
                _Node node = _ReadXMLNode( inputReader );
                AllNodes.insert(make_pair(node.id, node) );

                if ( node.trafficSignal )
                    SignalNodes.push_back( node.id );

            }
            else if ( xmlStrEqual( currentName, ( const xmlChar* ) "way" ) == 1 ) {
                stats.numberOfWays++;
                _Way way = _ReadXMLWay( inputReader );

                if ( way.usefull && way.access && way.path.size() ) {
                    for ( unsigned i = 0; i < way.path.size(); ++i ) {
                        UsedNodes.push_back( way.path[i] );
                    }

                    if ( way.direction == _Way::opposite )
                        std::reverse( way.path.begin(), way.path.end() );

                    stats.numberOfEdges += ( int ) way.path.size() - 1;

                    UsedWays.push_back(way);
                }
            }
            xmlFree( currentName );
        }
        sort(UsedNodes.begin(), UsedNodes.end());
        UsedNodes.erase(unique(UsedNodes.begin(), UsedNodes.end()), UsedNodes.end() );
        sort(SignalNodes.begin(), SignalNodes.end());
        SignalNodes.erase(unique(SignalNodes.begin(), SignalNodes.end()), SignalNodes.end() );
        cout << "ok" << endl;
        cout << endl << "Statistics: " << endl;
        cout << "All Nodes: " << stats.numberOfNodes << endl;
        cout << "Used Nodes: " << UsedNodes.size() << endl;
        cout << "Number of Ways: " << stats.numberOfWays << endl;
        cout << "Edges in graph: " << stats.numberOfEdges << endl;
        cout << "Number of ways with maxspeed information: " << stats.numberOfMaxspeed << endl;
        cout << "Number of nodes with traffic lights: " << SignalNodes.size() << endl;
        cout << "finished loading data" << endl;
        cout << "calculated edge weights and writing to disk ..." << flush;
        string name(argv[1]);
        int pos=name.find(".osm"); // pos=9
        if(pos!=string::npos)
        {
            //replace
            name.replace(pos, 5, ".osrm");
        } else {
            name.append(".osrm");
        }


        ofstream fout;
        fout.open(name.c_str());

        fout << UsedNodes.size() << endl;
        for(vector<NodeID>::size_type i = 0; i < UsedNodes.size(); i++)
        {
            NodeMap::iterator it = AllNodes.find(UsedNodes[i]);
            assert(it!=AllNodes.end());
            fout << UsedNodes[i] << " " << it->second.lon << " " << it->second.lat << "\n" << flush;
        }
        UsedNodes.clear();
        fout << stats.numberOfEdges << endl;
        for(vector<_Way>::size_type i = 0; i < UsedWays.size(); i++)
        {
            vector< NodeID > & path = UsedWays[i].path;
            double speed = UsedWays[i].maximumSpeed;
            assert(UsedWays[i].type > -1 || UsedWays[i].maximumSpeed != -1);
            assert(path.size()>0);

            for(vector< NodeID >::size_type n = 0; n < path.size()-1; n++)
            {
                //insert path[n], path[n+1]
                NodeMap::iterator startit = AllNodes.find(path[n]);
                if(startit == AllNodes.end())
                {
                    cerr << "Node " << path[n] << " missing albeit referenced in way. Edge skipped" << endl;
                    continue;
                }
                NodeMap::iterator targetit = AllNodes.find(path[n+1]);

                if(targetit == AllNodes.end())
                {
                    cerr << "Node << " << path[n+1] << "missing albeit reference in a way. Edge skipped" << endl;
                    continue;
                }
                double distance = ApproximateDistance(startit->second.lat, startit->second.lon, targetit->second.lat, targetit->second.lon);
                if(speed == -1)
                    speed = settings.speedProfile.speed[UsedWays[i].type];
                double weight = ( distance * 10. ) / (speed / 3.6);
                double intWeight = max(1, (int) weight);
                switch(UsedWays[i].direction)
                {
                case _Way::notSure:
                    fout << startit->first << " " << targetit->first << " " << max(1, (int)distance) << " " << 0 << " " << intWeight << "\n";
                    break;
                case _Way::oneway:
                    fout << startit->first << " " << targetit->first << " " << max(1, (int)distance) << " " << 1 << " " << intWeight << "\n";
                    break;
                case _Way::bidirectional:
                    fout << startit->first << " " << targetit->first << " " << max(1, (int)distance) << " " << 0 << " " << intWeight << "\n";
                    break;
                case _Way::opposite:
                    fout << startit->first << " " << targetit->first << " " << max(1, (int)distance) << " " << 1 << " " << intWeight << "\n";
                    break;
                default:
                    assert(false);
                    break;
                }
            }
        }
        fout.close();
        cout << "ok" << endl;
    } catch ( const std::exception& e ) {
        cerr <<  "Caught Execption:" << e.what() << endl;
        return false;
    }
    AllNodes.clear();
    SignalNodes.clear();
    UsedWays.clear();
    xmlFreeTextReader(inputReader);
    return true;
}

_Way _ReadXMLWay( xmlTextReaderPtr& inputReader ) {
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
                            stats.numberOfMaxspeed++;
                        } else {
                            xmlStrPrintf( buffer, 100, ( const xmlChar* ) "%.lf kmh", maxspeed );
                            if ( xmlStrEqual( value, buffer ) == 1 ) {
                                way.maximumSpeed = maxspeed;
                                stats.numberOfMaxspeed++;
                            } else {
                                xmlStrPrintf( buffer, 100, ( const xmlChar* ) "%.lfkmh", maxspeed );
                                if ( xmlStrEqual( value, buffer ) == 1 ) {
                                    way.maximumSpeed = maxspeed;
                                    stats.numberOfMaxspeed++;
                                } else {
                                    xmlStrPrintf( buffer, 100, ( const xmlChar* ) "%.lf km/h", maxspeed );
                                    if ( xmlStrEqual( value, buffer ) == 1 ) {
                                        way.maximumSpeed = maxspeed;
                                        stats.numberOfMaxspeed++;
                                    } else {
                                        xmlStrPrintf( buffer, 100, ( const xmlChar* ) "%.lfkm/h", maxspeed );
                                        if ( xmlStrEqual( value, buffer ) == 1 ) {
                                            way.maximumSpeed = maxspeed;
                                            stats.numberOfMaxspeed++;
                                        }
                                    }
                                }
                            }
                        }
                    } else {
//                        string key( ( const char* ) k );
                        //                        int index = -1;// settings.accessList.indexOf( key );
                        //                        if ( index != -1 ) { //&& index < way.accessPriority ) {
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
                        //                        }
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
        node.lat =  static_cast<NodeID>(100000*atof(( const char* ) attribute ) );
        xmlFree( attribute );
    }
    attribute = xmlTextReaderGetAttribute( inputReader, ( const xmlChar* ) "lon" );
    if ( attribute != NULL ) {
        node.lon =  static_cast<NodeID>(100000*atof(( const char* ) attribute ));
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
