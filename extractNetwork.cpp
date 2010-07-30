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
#include <google/sparse_hash_map>

#include "typedefs.h"
#include "DataStructures/extractorStructs.h"
using namespace std;
typedef google::dense_hash_map<NodeID, _Node> NodeMap;

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
                _Way way = _ReadXMLWay( inputReader, settings, stats );

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
            fout << UsedNodes[i] << " " << it->second.lon << " " << it->second.lat << "\n";
        }
        fout << flush;
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

