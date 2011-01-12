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
#ifdef STXXL_VERBOSE_LEVEL
#undef STXXL_VERBOSE_LEVEL
#endif
#define STXXL_VERBOSE_LEVEL -1000

#include <cassert>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <libxml/xmlreader.h>
#include <google/sparse_hash_map>
#include <stxxl.h>

#include "typedefs.h"
#include "DataStructures/InputReaderFactory.h"
#include "DataStructures/ExtractorStructs.h"
#include "DataStructures/PBFParser.h"
#include "DataStructures/XMLParser.h"

typedef google::dense_hash_map<NodeID, _Node> NodeMap;
typedef stxxl::vector<NodeID> STXXLNodeIDVector;
typedef stxxl::vector<_Node> STXXLNodeVector;
typedef stxxl::vector<_Edge> STXXLEdgeVector;
typedef stxxl::vector<string> STXXLStringVector;

NodeMap * nodeMap = new NodeMap();
StringMap * stringMap = new StringMap();
unsigned globalRelationCounter = 0;
Settings settings;

STXXLNodeIDVector usedNodes;
STXXLNodeVector allNodes;
STXXLNodeVector confirmedNodes;
STXXLEdgeVector allEdges;
STXXLEdgeVector confirmedEdges;
STXXLStringVector nameVector;

bool nodeFunction(_Node n) {
    allNodes.push_back(n);
    return true;
}
bool relationFunction(_Relation r) {
    globalRelationCounter++;
    return true;
}
bool wayFunction(_Way w) {
    std::string highway( w.keyVals.Find("highway") );
    std::string name( w.keyVals.Find("name") );
    std::string ref( w.keyVals.Find("ref"));
    std::string oneway( w.keyVals.Find("oneway"));
    std::string junction( w.keyVals.Find("junction") );
    std::string route( w.keyVals.Find("route") );
    std::string maxspeed( w.keyVals.Find("maxspeed") );
    std::string access( w.keyVals.Find("access") );
    std::string motorcar( w.keyVals.Find("motorcar") );

    if ( name != "" ) {
        w.name = name;
    } else if ( ref != "" ) {
        w.name = ref;
    }

    if ( oneway != "" ) {
        if ( oneway == "no" || oneway == "false" || oneway == "0" ) {
            w.direction = _Way::bidirectional;
        } else {
            if ( oneway == "yes" || oneway == "true" || oneway == "1" ) {
                w.direction = _Way::oneway;
            } else {
                if (oneway == "-1" )
                    w.direction = _Way::opposite;
            }
        }
    }
    if ( junction == "roundabout" ) {
        if ( w.direction == _Way::notSure ) {
            w.direction = _Way::oneway;
        }
        w.useful = true;
        if(w.type == -1)
            w.type = 9;
    }
    if ( route == "ferry") {
        for ( unsigned i = 0; i < settings.speedProfile.names.size(); i++ ) {
            if ( route == settings.speedProfile.names[i] ) {
                w.type = i;
                w.maximumSpeed = settings.speedProfile.speed[i];
                w.useful = true;
                w.direction = _Way::bidirectional;
                break;
            }
        }
    }
    if ( highway != "" ) {
        for ( unsigned i = 0; i < settings.speedProfile.names.size(); i++ ) {
            if ( highway == settings.speedProfile.names[i] ) {
                w.maximumSpeed = settings.speedProfile.speed[i];
                w.type = i;
                w.useful = true;
                break;
            }
        }
        if ( highway == "motorway"  ) {
            if ( w.direction == _Way::notSure ) {
                w.direction = _Way::oneway;
            }
        } else if ( highway == "motorway_link" ) {
            if ( w.direction == _Way::notSure ) {
                w.direction = _Way::oneway;
            }
        }
    }
    if ( maxspeed != "" ) {
        double maxspeedNumber = atof( maxspeed.c_str() );
        if(maxspeedNumber != 0) {
            w.maximumSpeed = maxspeedNumber;
        }
    }

    if ( access != "" ) {
        if ( access == "private"  || access == "no" || access == "agricultural" || access == "forestry" || access == "delivery") {
            w.access = false;
        }
        if ( access == "yes"  || access == "designated" || access == "official" || access == "permissive") {
            w.access = true;
        }
    }
    if ( motorcar == "yes" ) {
        w.access = true;
    } else if ( motorcar == "no" ) {
        w.access = false;
    }

    if ( w.useful && w.access && w.path.size() ) {
        //        std::cout << "[debug] looking for name: " << w.name << std::endl;
        StringMap::iterator strit = stringMap->find(w.name);
        if(strit == stringMap->end())
        {
            w.nameID = nameVector.size();
            nameVector.push_back(w.name);
            stringMap->insert(std::make_pair(w.name, w.nameID) );
            //            if(w.name != "")
            //                cout << "[debug] found new name ID: " << w.nameID << " (" << w.name << ")" << endl;
        } else {
            w.nameID = strit->second;
            //            std::cout << "[debug] name with ID " << w.nameID << " already existing (" << w.name << ")" << endl;
        }
        for ( unsigned i = 0; i < w.path.size(); ++i ) {
            //            std::cout << "[debug] using node " << w.path[i] << std::endl;
            usedNodes.push_back(w.path[i]);
        }

        if ( w.direction == _Way::opposite ){
            std::reverse( w.path.begin(), w.path.end() );
        }
        vector< NodeID > & path = w.path;
        assert(w.type > -1 || w.maximumSpeed != -1);
        assert(path.size()>0);

        if(w.maximumSpeed == -1)
            w.maximumSpeed = settings.speedProfile.speed[w.type];
        for(vector< NodeID >::size_type n = 0; n < path.size()-1; n++) {
            _Edge e;
            e.start = w.path[n];
            e.target = w.path[n+1];
            e.type = w.type;
            e.direction = w.direction;
            e.speed = w.maximumSpeed;
            e.nameID = w.nameID;
            allEdges.push_back(e);
        }
    }
    return true;
}

int main (int argc, char *argv[]) {
    if(argc <= 1) {
        std::cerr << "usage: " << endl << argv[0] << " <file.osm>" << std::endl;
        exit(-1);
    }

    std::cout << "[extractor] extracting data from input file " << argv[1] << std::endl;
    bool isPBF = false;
    std::string outputFileName(argv[1]);
    std::string::size_type pos = outputFileName.find(".osm.bz2");
    if(pos==string::npos) {
        pos = outputFileName.find(".osm.pbf");
        if(pos!=string::npos) {
//            std::cout << "[debug] found pbf file" << std::endl;
            isPBF = true;
        }
    }
    if(pos!=string::npos) {
        outputFileName.replace(pos, 8, ".osrm");
    } else {
        pos=outputFileName.find(".osm");
        if(pos!=string::npos) {
            outputFileName.replace(pos, 5, ".osrm");
        } else {
            outputFileName.append(".osrm");
        }
    }

    settings.speedProfile.names.insert(settings.speedProfile.names.begin(), names, names+14);
    settings.speedProfile.speed.insert(settings.speedProfile.speed.begin(), speeds, speeds+14);

    double time = get_timestamp();

    nodeMap->set_empty_key(UINT_MAX);
    stringMap->set_empty_key(GetRandomString());
    stringMap->insert(std::make_pair("", 0));
    BaseParser<_Node, _Relation, _Way> * parser;
    if(isPBF)
        parser = new PBFParser(argv[1]);
    else
        parser = new XMLParser(argv[1]);
    parser->RegisterCallbacks(&nodeFunction, &relationFunction, &wayFunction);
    if(parser->Init()) {
        parser->Parse();
    } else {
        std::cerr << "[error] parser not initialized!" << std::endl;
        exit(-1);
    }

    try {
        std::cout << "[info] raw no. of names: "       << nameVector.size()     << std::endl;
        std::cout << "[info] raw no. of nodes: "       << allNodes.size()       << std::endl;
        std::cout << "[info] no. of used nodes: "       << usedNodes.size()       << std::endl;
        std::cout << "[info] raw no. of edges: "       << allEdges.size()       << std::endl;
        std::cout << "[info] raw no. of relations: "   << globalRelationCounter << std::endl;

        std::cout << "[info] parsing throug input file took " << get_timestamp() - time << "seconds" << std::endl;
        time = get_timestamp();
        unsigned memory_to_use = 1024 * 1024 * 1024;

        std::cout << "[extractor] Sorting used nodes        ... " << std::flush;
        stxxl::sort(usedNodes.begin(), usedNodes.end(), Cmp(), memory_to_use);
        std::cout << "ok, after " << get_timestamp() - time << "s" << std::endl;
        time = get_timestamp();
        std::cout << "[extractor] Erasing duplicate entries ... " << std::flush;
        stxxl::vector<NodeID>::iterator NewEnd = unique ( usedNodes.begin(),usedNodes.end() ) ;
        usedNodes.resize ( NewEnd - usedNodes.begin() );
        cout << "ok, after " << get_timestamp() - time << "s" << endl;
        time = get_timestamp();

        std::cout << "[extractor] Sorting all nodes         ... " << std::flush;
        stxxl::sort(allNodes.begin(), allNodes.end(), CmpNodeByID(), memory_to_use);
        std::cout << "ok, after " << get_timestamp() - time << "s" << std::endl;
        time = get_timestamp();

        ofstream fout;
        fout.open(outputFileName.c_str());

        cout << "[extractor] Confirming used nodes     ... " << flush;
        STXXLNodeVector::iterator nvit = allNodes.begin();
        STXXLNodeIDVector::iterator niit = usedNodes.begin();
        while(niit != usedNodes.end() && nvit != allNodes.end()) {
            if(*niit < nvit->id){
                niit++;
                continue;
            }
            if(*niit > nvit->id) {
                nvit++;
                continue;
            }
            if(*niit == nvit->id) {
                confirmedNodes.push_back(*nvit);
                nodeMap->insert(std::make_pair(nvit->id, *nvit));
                niit++;
                nvit++;
            }
        }
        cout << "ok, after " << get_timestamp() - time << "s" << endl;
        time = get_timestamp();

        cout << "[extractor] Writing used nodes        ... " << flush;
        fout << confirmedNodes.size() << endl;
        for(STXXLNodeVector::iterator ut = confirmedNodes.begin(); ut != confirmedNodes.end(); ut++) {
            fout << ut->id<< " " << ut->lon << " " << ut->lat << "\n";
        }

        cout << "ok, after " << get_timestamp() - time << "s" << endl;
        time = get_timestamp();

        cout << "[extractor] confirming used ways      ... " << flush;
        for(STXXLEdgeVector::iterator eit = allEdges.begin(); eit != allEdges.end(); eit++) {
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

        cout << "[extractor] writing confirmed ways    ... " << flush;
        for(STXXLEdgeVector::iterator eit = confirmedEdges.begin(); eit != confirmedEdges.end(); eit++) {
            NodeMap::iterator startit = nodeMap->find(eit->start);
            if(startit == nodeMap->end()) {
                continue;
            }
            NodeMap::iterator targetit = nodeMap->find(eit->target);

            if(targetit == nodeMap->end()) {
                continue;
            }
            double distance = ApproximateDistance(startit->second.lat, startit->second.lon, targetit->second.lat, targetit->second.lon);
            if(eit->speed == -1)
                eit->speed = settings.speedProfile.speed[eit->type];
            double weight = ( distance * 10. ) / (eit->speed / 3.6);
            int intWeight = max(1, (int) weight);
            int intDist = max(1, (int)distance);
            int ferryIndex = settings.indexInAccessListOf("ferry");
            assert(ferryIndex != -1);

            switch(eit->direction) {
            case _Way::notSure:
                fout << startit->first << " " << targetit->first << " " << intDist << " " << 0 << " " << intWeight << " " << eit->type << " " << eit->nameID << "\n";
                break;
            case _Way::oneway:
                fout << startit->first << " " << targetit->first << " " << intDist << " " << 1 << " " << intWeight << " " << eit->type << " " << eit->nameID << "\n";
                break;
            case _Way::bidirectional:
                fout << startit->first << " " << targetit->first << " " << intDist << " " << 0 << " " << intWeight << " " << eit->type << " " << eit->nameID << "\n";
                break;
            case _Way::opposite:
                fout << startit->first << " " << targetit->first << " " << intDist << " " << 1 << " " << intWeight << " " << eit->type << " " << eit->nameID << "\n";
                break;
            default:
                std::cerr << "[error] edge with no direction: " << eit->direction << std::endl;
                assert(false);
                break;
            }
        }
        fout.close();

        outputFileName.append(".names");
        std::cout << "ok, after " << get_timestamp() - time << "s" << std::endl;
//        std::cout << "[debug] written edges: " << testCounter << std::endl;
        time = get_timestamp();
        std::cout << "[extractor] writing street name index ... " << std::flush;

        std::vector<unsigned> * nameIndex = new std::vector<unsigned>(nameVector.size()+1, 0);
        unsigned currentNameIndex = 0;
        for(unsigned i = 0; i < nameVector.size(); i++) {
            nameIndex->at(i) = currentNameIndex;
            currentNameIndex += nameVector[i].length();
        }
        nameIndex->at(nameVector.size()) = currentNameIndex;
        ofstream nameOutFile(outputFileName.c_str(), ios::binary);
        unsigned sizeOfNameIndex = nameIndex->size();
        nameOutFile.write((char *)&(sizeOfNameIndex), sizeof(unsigned));

        for(unsigned i = 0; i < nameIndex->size(); i++) {
            nameOutFile.write((char *)&(nameIndex->at(i)), sizeof(unsigned));
        }
        for(unsigned i = 0; i < nameVector.size(); i++){
            nameOutFile << nameVector[i];
        }

        nameOutFile.close();
        delete nameIndex;
        std::cout << "ok, after " << get_timestamp() - time << "s" << std::endl;

    } catch ( const std::exception& e ) {
        std::cerr <<  "Caught Execption:" << e.what() << std::endl;
        return false;
    }

    std::cout << "[info] Statistics:" << std::endl;
    std::cout << "[info] -----------" << std::endl;
    std::cout << "[info] Usable Nodes: " << confirmedNodes.size() << std::endl;
    std::cout << "[info] Usable Edges: " << confirmedEdges.size() << std::endl;

    usedNodes.clear();
    allNodes.clear();
    confirmedNodes.clear();
    allEdges.clear();
    confirmedEdges.clear();
    nameVector.clear();
    delete nodeMap;
    delete stringMap;
    delete parser;
    cout << "[extractor] finished." << endl;
    return 0;
}
