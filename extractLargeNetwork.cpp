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
#include "DataStructures/ExtractorCallBacks.h"
#include "DataStructures/ExtractorStructs.h"
#include "DataStructures/PBFParser.h"
#include "DataStructures/XMLParser.h"

unsigned globalRelationCounter = 0;
ExtractorCallbacks * extractCallBacks;

bool nodeFunction(_Node n);
bool adressFunction(_Node n, HashTable<std::string, std::string> keyVals);
bool relationFunction(_Relation r);
bool wayFunction(_Way w);

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
    std::string adressFileName(outputFileName);

    STXXLNodeIDVector * usedNodes       = new STXXLNodeIDVector();
    STXXLNodeVector   * allNodes        = new STXXLNodeVector();
    STXXLNodeVector   * confirmedNodes  = new STXXLNodeVector();
    STXXLEdgeVector   * allEdges        = new STXXLEdgeVector();
    STXXLEdgeVector   * confirmedEdges  = new STXXLEdgeVector();
    STXXLAddressVector* adressVector    = new STXXLAddressVector();
    STXXLStringVector * nameVector      = new STXXLStringVector();

    NodeMap * nodeMap = new NodeMap();
    StringMap * stringMap = new StringMap();
    Settings settings;
    settings.speedProfile.names.insert(settings.speedProfile.names.begin(), names, names+14);
    settings.speedProfile.speed.insert(settings.speedProfile.speed.begin(), speeds, speeds+14);

    double time = get_timestamp();

    nodeMap->set_empty_key(UINT_MAX);
    stringMap->set_empty_key(GetRandomString());
    stringMap->insert(std::make_pair("", 0));
    extractCallBacks = new ExtractorCallbacks(allNodes, usedNodes, allEdges,  nameVector, adressVector, settings, stringMap);

    BaseParser<_Node, _Relation, _Way> * parser;
    if(isPBF) {
        parser = new PBFParser(argv[1]);
    } else {
        parser = new XMLParser(argv[1]);
    }
    parser->RegisterCallbacks(&nodeFunction, &relationFunction, &wayFunction, &adressFunction);
    if(parser->Init()) {
        parser->Parse();
    } else {
        std::cerr << "[error] parser not initialized!" << std::endl;
        exit(-1);
    }

    try {
        std::cout << "[info] raw no. of names:     " << nameVector->size()    << std::endl;
        std::cout << "[info] raw no. of nodes:     " << allNodes->size()      << std::endl;
        std::cout << "[info] no. of used nodes:    " << usedNodes->size()     << std::endl;
        std::cout << "[info] raw no. of edges:     " << allEdges->size()      << std::endl;
        std::cout << "[info] raw no. of relations: " << globalRelationCounter << std::endl;
        std::cout << "[info] raw no. of addresses: " << adressVector->size()  << std::endl;

        std::cout << "[info] parsing through input file took " << get_timestamp() - time << "seconds" << std::endl;
        time = get_timestamp();
        unsigned memory_to_use = 1024 * 1024 * 1024;

        std::cout << "[extractor] Sorting used nodes        ... " << std::flush;
        stxxl::sort(usedNodes->begin(), usedNodes->end(), Cmp(), memory_to_use);
        std::cout << "ok, after " << get_timestamp() - time << "s" << std::endl;
        std::cout << "[debug] highest node id: " << usedNodes->back() << std::endl;

        time = get_timestamp();
        std::cout << "[extractor] Erasing duplicate entries ... " << std::flush;
        stxxl::vector<NodeID>::iterator NewEnd = unique ( usedNodes->begin(),usedNodes->end() ) ;
        usedNodes->resize ( NewEnd - usedNodes->begin() );
        cout << "ok, after " << get_timestamp() - time << "s" << endl;
        time = get_timestamp();

        std::cout << "[extractor] Sorting all nodes         ... " << std::flush;
        stxxl::sort(allNodes->begin(), allNodes->end(), CmpNodeByID(), memory_to_use);
        std::cout << "ok, after " << get_timestamp() - time << "s" << std::endl;
        time = get_timestamp();

        ofstream fout;
        fout.open(outputFileName.c_str());

        cout << "[extractor] Confirming used nodes     ... " << flush;
        STXXLNodeVector::iterator nvit = allNodes->begin();
        STXXLNodeIDVector::iterator niit = usedNodes->begin();
        while(niit != usedNodes->end() && nvit != allNodes->end()) {
            if(*niit < nvit->id){
                niit++;
                continue;
            }
            if(*niit > nvit->id) {
                nvit++;
                continue;
            }
            if(*niit == nvit->id) {
                confirmedNodes->push_back(*nvit);
                nodeMap->insert(std::make_pair(nvit->id, *nvit));
                niit++;
                nvit++;
            }
        }
        cout << "ok, after " << get_timestamp() - time << "s" << endl;
        std::cout << "[debug] no of entries in nodemap" << nodeMap->size() << std::endl;
        time = get_timestamp();

        cout << "[extractor] Writing used nodes        ... " << flush;
        fout << confirmedNodes->size() << endl;
        for(STXXLNodeVector::iterator ut = confirmedNodes->begin(); ut != confirmedNodes->end(); ut++) {
            fout << ut->id<< " " << ut->lon << " " << ut->lat << "\n";
        }

        cout << "ok, after " << get_timestamp() - time << "s" << endl;
        time = get_timestamp();

        cout << "[extractor] confirming used ways      ... " << flush;
        for(STXXLEdgeVector::iterator eit = allEdges->begin(); eit != allEdges->end(); eit++) {
            assert(eit->type > -1 || eit->speed != -1);

            NodeMap::iterator startit = nodeMap->find(eit->start);
            if(startit == nodeMap->end()) {
                continue;
            }
            NodeMap::iterator targetit = nodeMap->find(eit->target);

            if(targetit == nodeMap->end()) {
                continue;
            }
            confirmedEdges->push_back(*eit);
        }
        fout << confirmedEdges->size() << "\n";
        cout << "ok, after " << get_timestamp() - time << "s" << endl;
        time = get_timestamp();

        cout << "[extractor] writing confirmed ways    ... " << flush;
        for(STXXLEdgeVector::iterator eit = confirmedEdges->begin(); eit != confirmedEdges->end(); eit++) {
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

        std::vector<unsigned> * nameIndex = new std::vector<unsigned>(nameVector->size()+1, 0);
        unsigned currentNameIndex = 0;
        unsigned elementCounter(0);
        for(STXXLStringVector::iterator it = nameVector->begin(); it != nameVector->end(); it++) {
            nameIndex->at(elementCounter) = currentNameIndex;
            currentNameIndex += it->length();
            elementCounter++;
        }
        nameIndex->at(nameVector->size()) = currentNameIndex;
        ofstream nameOutFile(outputFileName.c_str(), ios::binary);
        unsigned sizeOfNameIndex = nameIndex->size();
        nameOutFile.write((char *)&(sizeOfNameIndex), sizeof(unsigned));

        for(unsigned i = 0; i < nameIndex->size(); i++) {
            nameOutFile.write((char *)&(nameIndex->at(i)), sizeof(unsigned));
        }
        for(STXXLStringVector::iterator it = nameVector->begin(); it != nameVector->end(); it++) {
            nameOutFile << *it;
        }

        nameOutFile.close();
        delete nameIndex;
        std::cout << "ok, after " << get_timestamp() - time << "s" << std::endl;

        time = get_timestamp();
        std::cout << "[extractor] writing address list      ... " << std::flush;

        adressFileName.append(".address");
        std::ofstream addressOutFile(adressFileName.c_str());
        for(STXXLAddressVector::iterator it = adressVector->begin(); it != adressVector->end(); it++) {
            addressOutFile << it->node.id << "|" << it->node.lat << "|" << it->node.lon << "|" << it->city << "|" << it->street << "|" << it->housenumber << "|" << it->state << "|" << it->country << "\n";
        }
        addressOutFile.close();
        std::cout << "ok, after " << get_timestamp() - time << "s" << std::endl;

    } catch ( const std::exception& e ) {
        std::cerr <<  "Caught Execption:" << e.what() << std::endl;
        return false;
    }

    std::cout << "[info] Statistics:" << std::endl;
    std::cout << "[info] -----------" << std::endl;
    std::cout << "[info] Usable Nodes: " << confirmedNodes->size() << std::endl;
    std::cout << "[info] Usable Edges: " << confirmedEdges->size() << std::endl;

    delete extractCallBacks;
    delete nodeMap;
    delete confirmedNodes;
    delete confirmedEdges;
    delete adressVector;
    delete parser;
    cout << "[extractor] finished." << endl;
    return 0;
}

bool nodeFunction(_Node n) {
    extractCallBacks->nodeFunction(n);
    return true;
}

bool adressFunction(_Node n, HashTable<std::string, std::string> keyVals){
    extractCallBacks->adressFunction(n, keyVals);
    return true;
}

bool relationFunction(_Relation r) {
    globalRelationCounter++;
    return true;
}
bool wayFunction(_Way w) {
    extractCallBacks->wayFunction(w);
    return true;
}

