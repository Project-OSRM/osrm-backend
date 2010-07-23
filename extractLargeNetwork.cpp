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

#include <stxxl.h>

#include "typedefs.h"
#include "DataStructures/extractorStructs.h"

using namespace std;
typedef google::dense_hash_map<NodeID, _Node> NodeMap;

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
	double time = get_timestamp();
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
				_Way way = _ReadXMLWay( inputReader, settings, stats );

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
							//serialize edge (path[n], path[n+1])
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
		cout << "ok, after " << get_timestamp() - time << "s" << endl;
		time = get_timestamp();
		unsigned memory_to_use = 1024 * 1024 * 1024;

		stxxl::syscall_file f("_usednodes", stxxl::file::DIRECT | stxxl::file::RDWR);
		typedef stxxl::vector<NodeID> usedNodesVectorType;

		usedNodesVectorType usedNodes( &f);
		cout << "Sorting used nodes ..." << flush;
		stxxl::sort(usedNodes.begin(), usedNodes.end(), Cmp(), memory_to_use);
		cout << "ok, after " << get_timestamp() - time << "s" << endl;
		time = get_timestamp();
		cout << "Erasing duplicate entries ..." << flush;
		stxxl::vector<NodeID>::iterator NewEnd = unique ( usedNodes.begin(),usedNodes.end() ) ;
		usedNodes.resize ( NewEnd - usedNodes.begin() );
		cout << "ok, after " << get_timestamp() - time << "s" << endl;
		time = get_timestamp();

		stxxl::syscall_file fallnodes("_allnodes", stxxl::file::DIRECT | stxxl::file::RDWR);
		typedef stxxl::vector< _Node > second_vector_type;

		second_vector_type van(&fallnodes);
		cout << "Sorting all nodes ..." << flush;
		stxxl::ksort(van.begin(), van.end(), memory_to_use);
		cout << "ok, after " << get_timestamp() - time << "s" << endl;
		time = get_timestamp();

		cout << endl << "Statistics: " << endl;
		cout << "All Nodes: " << stats.numberOfNodes << endl;
		cout << "Used Nodes: " << nodeMap->size() << endl;
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
		cout << "ok, after " << get_timestamp() - time << "s" << endl;
		time = get_timestamp();

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
		cout << "ok, after " << get_timestamp() - time << "s" << endl;
		time = get_timestamp();
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

