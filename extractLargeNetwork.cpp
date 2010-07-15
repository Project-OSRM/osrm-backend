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
#include <cstdio>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <libxml/xmlreader.h>

#include <google/sparse_hash_map>

#include <stxxl/mng>
#include <stxxl/ksort>
#include <stxxl/sort>
#include <stxxl/vector>
#include <stxxl/bits/io/syscall_file.h>

#include "typedefs.h"

using namespace std;

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

struct _Stats {
	NodeID numberOfNodes;
	NodeID numberOfEdges;
	NodeID numberOfWays;
	NodeID numberOfMaxspeed;
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
	typedef unsigned value_type;
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

typedef google::dense_hash_map<NodeID, _Node> NodeMap;

_Way _ReadXMLWay( xmlTextReaderPtr& inputReader );
_Node _ReadXMLNode( xmlTextReaderPtr& inputReader );
double ApproximateDistance( const  int lat1, const  int lon1, const  int lat2, const  int lon2 );

_Stats stats;
Settings settings;

vector<NodeID> SignalNodes;
NodeMap * nodeMap = new NodeMap();
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

	xmlTextReaderPtr inputReader = xmlNewTextReaderFilename( argv[1] );
	ofstream allNodeFile("_allnodes", ios::binary);
	ofstream usedNodesFile("_usednodes", ios::binary);
	ofstream wayFile("_ways", ios::binary);
	nodeMap->set_empty_key(UINT_MAX);
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
				allNodeFile.write((char *)&node, sizeof(node));// << node.id << node.lat << node.lon << node.trafficSignal << endl;

				if ( node.trafficSignal )
					SignalNodes.push_back( node.id );

			}
			else if ( xmlStrEqual( currentName, ( const xmlChar* ) "way" ) == 1 ) {
				stats.numberOfWays++;
				_Way way = _ReadXMLWay( inputReader );

				if ( way.usefull && way.access && way.path.size() ) {
					for ( unsigned i = 0; i < way.path.size(); ++i ) {
						usedNodesFile.write((char *)&way.path[i], sizeof(NodeID));
					}

					if ( way.direction == _Way::opposite )
						std::reverse( way.path.begin(), way.path.end() );

					stats.numberOfEdges += ( int ) way.path.size() - 1;
					{
						vector< NodeID > & path = way.path;
						double speed = way.maximumSpeed;
						assert(way.type > -1 || way.maximumSpeed != -1);
						assert(path.size()>0);

						for(vector< NodeID >::size_type n = 0; n < path.size()-1; n++)
						{
							//serialize path[n], path[n+1]
							wayFile.write((char*)&way.path[n], sizeof(NodeID));
							wayFile.write((char*)&way.path[n+1], sizeof(NodeID));
							wayFile.write((char*)&way.type, sizeof(short));
							wayFile.write((char*)&way.direction, sizeof(short));
							wayFile.write((char*)&way.maximumSpeed, sizeof(double));
						}
					}
				}
			}
			xmlFree( currentName );
		}
		allNodeFile.close();
		usedNodesFile.close();
		wayFile.close();
		cout << "ok" << endl;
		//sort(UsedNodes.begin(), UsedNodes.end());
		unsigned memory_to_use = 1024 * 1024 * 1024;

		stxxl::syscall_file f("_usednodes", stxxl::file::DIRECT | stxxl::file::RDWR);
		typedef stxxl::vector<NodeID> usedNodesVectorType;

		usedNodesVectorType usedNodes( &f);
		cout << "Sorting used nodes ..." << flush;
		stxxl::sort(usedNodes.begin(), usedNodes.end(), Cmp(), memory_to_use);
		cout << "ok" << endl;
		cout << "Erasing duplicate entries ..." << flush;
		stxxl::vector<NodeID>::iterator NewEnd = unique ( usedNodes.begin(),usedNodes.end() ) ;
		usedNodes.resize ( NewEnd - usedNodes.begin() );
		cout << "ok" << endl;

		stxxl::syscall_file fallnodes("_allnodes", stxxl::file::DIRECT | stxxl::file::RDWR);
		typedef stxxl::vector< _Node > second_vector_type;

		second_vector_type van(&fallnodes);
		cout << "Sorting all nodes ..." << flush;
		stxxl::ksort(van.begin(), van.end(), memory_to_use);
		cout << "ok" << endl;


		//        cout << endl << "Statistics: " << endl;
		//        cout << "All Nodes: " << stats.numberOfNodes << endl;
		//        //        cout << "Used Nodes: " << UsedNodes.size() << endl;
		//        cout << "Number of Ways: " << stats.numberOfWays << endl;
		//        cout << "Edges in graph: " << stats.numberOfEdges << endl;
		//        cout << "Number of ways with maxspeed information: " << stats.numberOfMaxspeed << endl;
		//        cout << "Number of nodes with traffic lights: " << SignalNodes.size() << endl;
		//        cout << "finished loading data" << endl;
		//        cout << "calculated edge weights and writing to disk ..." << flush;
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
		ifstream inall("_allnodes", ios::binary);
		ifstream inuse("_usednodes", ios::binary);
		ifstream inway("_ways", ios::binary);

		cout << "Writing used nodes ..." << flush;
		fout << usedNodes.size() << endl;
		NodeID counter = 0;
		for(usedNodesVectorType::iterator it = usedNodes.begin(); it!=usedNodes.end(); it++)
		{
			NodeID currentNodeID = *it;
			_Node current_Node;
			inall.read((char *)&current_Node, sizeof(_Node));
			while(currentNodeID!=current_Node.id)
			{
				inall.read((char *)&current_Node, sizeof(_Node));
			}
			fout << current_Node.id<< " " << current_Node.lon << " " << current_Node.lat << "\n";
			nodeMap->insert(std::make_pair(current_Node.id, current_Node));
			counter++;
		}
		cout << "ok" << endl;

		cout << "writing used ways ..." << endl;
		NodeID start, target;
		short type, direction;
		double maximumSpeed;
		fout << stats.numberOfEdges << "\n";
		while(!inway.eof())
		{

			inway.read((char*)&start, sizeof(NodeID));
			inway.read((char*)&target, sizeof(NodeID));
			inway.read((char*)&type, sizeof(short));
			inway.read((char*)&direction, sizeof(short));
			inway.read((char*)&maximumSpeed, sizeof(double));
			assert(type > -1 || maximumSpeed != -1);

			NodeMap::iterator startit = nodeMap->find(start);
			if(startit == nodeMap->end())
			{
				cerr << "Node " << start << " missing albeit referenced in way. Edge skipped" << endl;
				continue;
			}
			NodeMap::iterator targetit = nodeMap->find(target);

			if(targetit == nodeMap->end())
			{
				cerr << "Node << " << target << "missing albeit reference in a way. Edge skipped" << endl;
				continue;
			}

			double distance = ApproximateDistance(startit->second.lat, startit->second.lon, targetit->second.lat, targetit->second.lon);
			if(maximumSpeed == -1)
				maximumSpeed = settings.speedProfile.speed[type];
			double weight = ( distance * 10. ) / (maximumSpeed / 3.6);
			double intWeight = max(1, (int) weight);
			switch(direction)
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
		inway.close();
		fout.close();
		cout << "ok" << endl;
	} catch ( const std::exception& e ) {
		cerr <<  "Caught Execption:" << e.what() << endl;
		return false;
	}
	SignalNodes.clear();
	xmlFreeTextReader(inputReader);

	remove("_allnodes");
	remove("_usednodes");
	remove("_ways");
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
