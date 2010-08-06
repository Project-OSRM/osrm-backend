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
typedef stxxl::vector<NodeID> STXXLNodeIDVector;
typedef stxxl::vector<_Node> STXXLNodeVector;
typedef stxxl::vector<_Edge> STXXLEdgeVector;

Settings settings;
vector<NodeID> SignalNodes;
STXXLNodeIDVector usedNodes;
STXXLNodeVector allNodes;
STXXLNodeVector confirmedNodes;
STXXLEdgeVector allEdges;
STXXLEdgeVector confirmedEdges;
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
                _Node node = _ReadXMLNode( inputReader );
                allNodes.push_back(node);
                if ( node.trafficSignal )
                    SignalNodes.push_back( node.id );

            }
            else if ( xmlStrEqual( currentName, ( const xmlChar* ) "way" ) == 1 ) {
                _Way way = _ReadXMLWay( inputReader, settings );

                if ( way.usefull && way.access && way.path.size() ) {
                    for ( unsigned i = 0; i < way.path.size(); ++i ) {
                        usedNodes.push_back(way.path[i]);
                    }

                    if ( way.direction == _Way::opposite )
                        std::reverse( way.path.begin(), way.path.end() );

                    {
                        vector< NodeID > & path = way.path;
                        double speed = way.maximumSpeed;
                        assert(way.type > -1 || way.maximumSpeed != -1);
                        assert(path.size()>0);

                        for(vector< NodeID >::size_type n = 0; n < path.size()-1; n++)
                        {
                            _Edge e;
                            e.start = way.path[n];
                            e.target = way.path[n+1];
                            e.type = way.type;
                            e.direction = way.direction;
                            e.speed = way.maximumSpeed;
                            allEdges.push_back(e);
                        }
                    }
                }
            }
            xmlFree( currentName );
        }
        cout << "ok, after " << get_timestamp() - time << "s" << endl;
        time = get_timestamp();
        unsigned memory_to_use = 1024 * 1024 * 1024;

        cout << "Sorting used nodes ..." << flush;
        stxxl::sort(usedNodes.begin(), usedNodes.end(), Cmp(), memory_to_use);
        cout << "ok, after " << get_timestamp() - time << "s" << endl;
        time = get_timestamp();
        cout << "Erasing duplicate entries ..." << flush;
        stxxl::vector<NodeID>::iterator NewEnd = unique ( usedNodes.begin(),usedNodes.end() ) ;
        usedNodes.resize ( NewEnd - usedNodes.begin() );
        cout << "ok, after " << get_timestamp() - time << "s" << endl;
        time = get_timestamp();

        cout << "Sorting all nodes ..." << flush;
        stxxl::ksort(allNodes.begin(), allNodes.end(), memory_to_use);
        cout << "ok, after " << get_timestamp() - time << "s" << endl;
        time = get_timestamp();

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
        //        ifstream inway("_ways", ios::binary);

        cout << "Writing used nodes ..." << flush;
        NodeID counter = 0;
        NodeID notfound = 0;
        STXXLNodeVector::iterator nvit = allNodes.begin();
        STXXLNodeIDVector::iterator niit = usedNodes.begin();
        while(niit != usedNodes.end() && nvit != allNodes.end())
        {
            if(*niit < nvit->id){
                niit++;
                continue;
            }
            if(*niit > nvit->id)
            {
                nvit++;
                continue;
            }
            if(*niit == nvit->id)
            {
                confirmedNodes.push_back(*nvit);
                nodeMap->insert(std::make_pair(nvit->id, *nvit));
                niit++;
                nvit++;
            }
        }
        fout << confirmedNodes.size() << endl;
        for(STXXLNodeVector::iterator ut = confirmedNodes.begin(); ut != confirmedNodes.end(); ut++)
        {
            fout << ut->id<< " " << ut->lon << " " << ut->lat << "\n";
        }

        cout << "ok, after " << get_timestamp() - time << "s" << endl;
        time = get_timestamp();

        cout << "confirming used ways ..." << endl;
        for(STXXLEdgeVector::iterator eit = allEdges.begin(); eit != allEdges.end(); eit++)
        {
            assert(eit->type > -1 || eit->speed != -1);

            NodeMap::iterator startit = nodeMap->find(eit->start);
            if(startit == nodeMap->end())
            {
                continue;
            }
            NodeMap::iterator targetit = nodeMap->find(eit->target);

            if(targetit == nodeMap->end())
            {
                continue;
            }
            confirmedEdges.push_back(*eit);
        }
        fout << confirmedEdges.size() << "\n";
        cout << "ok, after " << get_timestamp() - time << "s" << endl;
        time = get_timestamp();

        cout << "writing confirmed ways ..." << endl;

        for(STXXLEdgeVector::iterator eit = confirmedEdges.begin(); eit != confirmedEdges.end(); eit++)
        {
            NodeMap::iterator startit = nodeMap->find(eit->start);
            if(startit == nodeMap->end())
            {
                continue;
            }
            NodeMap::iterator targetit = nodeMap->find(eit->target);

            if(targetit == nodeMap->end())
            {
                continue;
            }
            double distance = ApproximateDistance(startit->second.lat, startit->second.lon, targetit->second.lat, targetit->second.lon);
            if(eit->speed == -1)
                eit->speed = settings.speedProfile.speed[eit->type];
            double weight = ( distance * 10. ) / (eit->speed / 3.6);
            double intWeight = max(1, (int) weight);
            switch(eit->direction)
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
        fout.close();
        cout << "ok, after " << get_timestamp() - time << "s" << endl;
        time = get_timestamp();
    } catch ( const std::exception& e ) {
        cerr <<  "Caught Execption:" << e.what() << endl;
        return false;
    }

    cout << endl << "Statistics:" << endl;
    cout << "-----------" << endl;
    cout << "Usable Nodes: " << confirmedNodes.size() << endl;
    cout << "Usable Ways : " << confirmedEdges.size() << endl;

    SignalNodes.clear();
    usedNodes.clear();
    allNodes.clear();
    confirmedNodes.clear();
    allEdges.clear();
    confirmedEdges.clear();

    xmlFreeTextReader(inputReader);
    return 0;
}

