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

#include <algorithm>
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
#include <unistd.h>
#include <stxxl.h>

#include "typedefs.h"
#include "DataStructures/InputReaderFactory.h"
#include "DataStructures/ExtractorCallBacks.h"
#include "DataStructures/ExtractorStructs.h"
#include "DataStructures/PBFParser.h"
#include "DataStructures/XMLParser.h"
#include "Util/BaseConfiguration.h"
#include "Util/InputFileUtil.h"

typedef BaseConfiguration ExtractorConfiguration;

unsigned globalRelationCounter = 0;
ExtractorCallbacks * extractCallBacks;

bool nodeFunction(_Node n);
bool adressFunction(_Node n, HashTable<std::string, std::string> keyVals);
bool relationFunction(_Relation r);
bool wayFunction(_Way w);

template<class ClassT>
bool removeIfUnused(ClassT n) { return (false == n.used); }

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



    unsigned amountOfRAM = 1;
    unsigned installedRAM = (sysconf(_SC_PHYS_PAGES) * sysconf(_SC_PAGESIZE));
    if(installedRAM < 2097422336) {
        std::cout << "[Warning] Machine has less than 2GB RAM." << std::endl;
    }
    if(testDataFile("extractor.ini")) {
        ExtractorConfiguration extractorConfig("extractor.ini");
        unsigned memoryAmountFromFile = atoi(extractorConfig.GetParameter("Memory").c_str());
        if( memoryAmountFromFile != 0 && memoryAmountFromFile <= installedRAM/(1024*1024*1024))
            amountOfRAM = memoryAmountFromFile;
        std::cout << "[extractor] using " << amountOfRAM << " GB of RAM for buffers" << std::endl;
    }

    STXXLNodeIDVector usedNodeIDs;
    STXXLNodeVector   allNodes;
    STXXLEdgeVector   allEdges;
    STXXLAddressVector adressVector;
    STXXLStringVector  nameVector;
    unsigned usedNodeCounter = 0;
    unsigned usedEdgeCounter = 0;

    StringMap * stringMap = new StringMap();
    Settings settings;
    settings.speedProfile.names.insert(settings.speedProfile.names.begin(), names, names+14);
    settings.speedProfile.speed.insert(settings.speedProfile.speed.begin(), speeds, speeds+14);

    double time = get_timestamp();

    stringMap->set_empty_key(GetRandomString());
    stringMap->insert(std::make_pair("", 0));
    extractCallBacks = new ExtractorCallbacks(&allNodes, &usedNodeIDs, &allEdges,  &nameVector, &adressVector, settings, stringMap);

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
    delete parser;

    try {
        //        std::cout << "[info] raw no. of names:     " << nameVector.size()    << std::endl;
        //        std::cout << "[info] raw no. of nodes:     " << allNodes.size()      << std::endl;
        //        std::cout << "[info] no. of used nodes:    " << usedNodeIDs.size()     << std::endl;
        //        std::cout << "[info] raw no. of edges:     " << allEdges.size()      << std::endl;
        //        std::cout << "[info] raw no. of relations: " << globalRelationCounter << std::endl;
        //        std::cout << "[info] raw no. of addresses: " << adressVector.size()  << std::endl;

        std::cout << "[extractor] parsing finished after " << get_timestamp() - time << "seconds" << std::endl;
        time = get_timestamp();
        unsigned memory_to_use = amountOfRAM * 1024 * 1024 * 1024;

        std::cout << "[extractor] Sorting used nodes        ... " << std::flush;
        stxxl::sort(usedNodeIDs.begin(), usedNodeIDs.end(), Cmp(), memory_to_use);
        std::cout << "ok, after " << get_timestamp() - time << "s" << std::endl;

        time = get_timestamp();
        std::cout << "[extractor] Erasing duplicate nodes   ... " << std::flush;
        stxxl::vector<NodeID>::iterator NewEnd = unique ( usedNodeIDs.begin(),usedNodeIDs.end() ) ;
        usedNodeIDs.resize ( NewEnd - usedNodeIDs.begin() );
        cout << "ok, after " << get_timestamp() - time << "s" << endl;
        time = get_timestamp();

        std::cout << "[extractor] Sorting all nodes         ... " << std::flush;
        stxxl::sort(allNodes.begin(), allNodes.end(), CmpNodeByID(), memory_to_use);
        std::cout << "ok, after " << get_timestamp() - time << "s" << std::endl;
        time = get_timestamp();

        std::ofstream fout;
        fout.open(outputFileName.c_str(), std::ios::binary);
        fout.write((char*)&usedNodeCounter, sizeof(unsigned));

        std::cout << "[extractor] Confirming used nodes     ... " << std::flush;
        STXXLNodeVector::iterator nodesIT = allNodes.begin();
        STXXLNodeIDVector::iterator usedNodeIDsIT = usedNodeIDs.begin();
        while(usedNodeIDsIT != usedNodeIDs.end() && nodesIT != allNodes.end()) {
            if(*usedNodeIDsIT < nodesIT->id){
                usedNodeIDsIT++;
                continue;
            }
            if(*usedNodeIDsIT > nodesIT->id) {
                nodesIT++;
                continue;
            }
            if(*usedNodeIDsIT == nodesIT->id) {
                fout.write((char*)&(nodesIT->id), sizeof(unsigned));
                fout.write((char*)&(nodesIT->lon), sizeof(int));
                fout.write((char*)&(nodesIT->lat), sizeof(int));
                usedNodeCounter++;
                usedNodeIDsIT++;
                nodesIT++;
            }
        }
        std::cout << "ok, after " << get_timestamp() - time << "s" << std::endl;
        time = get_timestamp();

        std::cout << "[extractor] setting number of nodes   ... " << std::flush;
        std::ios::pos_type positionInFile = fout.tellp();
        fout.seekp(std::ios::beg);
        fout.write((char*)&usedNodeCounter, sizeof(unsigned));
        fout.seekp(positionInFile);

        std::cout << "ok" << std::endl;
        time = get_timestamp();

        // Sort edges by start.
        std::cout << "[extractor] Sorting edges by start    ... " << std::flush;
        stxxl::sort(allEdges.begin(), allEdges.end(), CmpEdgeByStartID(), memory_to_use);
        std::cout << "ok, after " << get_timestamp() - time << "s" << std::endl;
        time = get_timestamp();

        std::cout << "[extractor] Setting start coords      ... " << std::flush;
        fout.write((char*)&usedEdgeCounter, sizeof(unsigned));
        // Traverse list of edges and nodes in parallel and set start coord
        nodesIT = allNodes.begin();
        STXXLEdgeVector::iterator edgeIT = allEdges.begin();
        while(edgeIT != allEdges.end() && nodesIT != allNodes.end()) {
            if(edgeIT->start < nodesIT->id){
                edgeIT++;
                continue;
            }
            if(edgeIT->start > nodesIT->id) {
                nodesIT++;
                continue;
            }
            if(edgeIT->start == nodesIT->id) {
                edgeIT->startCoord.lat = nodesIT->lat;
                edgeIT->startCoord.lon = nodesIT->lon;
                edgeIT++;
            }
        }
        std::cout << "ok, after " << get_timestamp() - time << "s" << std::endl;
        time = get_timestamp();

        // Sort Edges by target
        std::cout << "[extractor] Sorting edges by target   ... " << std::flush;
        stxxl::sort(allEdges.begin(), allEdges.end(), CmpEdgeByTargetID(), memory_to_use);
        std::cout << "ok, after " << get_timestamp() - time << "s" << std::endl;
        time = get_timestamp();

        std::cout << "[extractor] Setting target coords     ... " << std::flush;
        // Traverse list of edges and nodes in parallel and set target coord
        nodesIT = allNodes.begin();
        edgeIT = allEdges.begin();
        while(edgeIT != allEdges.end() && nodesIT != allNodes.end()) {
            if(edgeIT->target < nodesIT->id){
                edgeIT++;
                continue;
            }
            if(edgeIT->target > nodesIT->id) {
                nodesIT++;
                continue;
            }
            if(edgeIT->target == nodesIT->id) {
                if(edgeIT->startCoord.lat != INT_MIN) {
                    edgeIT->targetCoord.lat = nodesIT->lat;
                    edgeIT->targetCoord.lon = nodesIT->lon;

                    double distance = ApproximateDistance(edgeIT->startCoord.lat, edgeIT->startCoord.lon, nodesIT->lat, nodesIT->lon);
                    if(edgeIT->speed == -1)
                        edgeIT->speed = settings.speedProfile.speed[edgeIT->type];
                    double weight = ( distance * 10. ) / (edgeIT->speed / 3.6);
                    int intWeight = max(1, (int) weight);
                    int intDist = max(1, (int)distance);
                    int ferryIndex = settings.indexInAccessListOf("ferry");
                    assert(ferryIndex != -1);
                    short zero = 0;
                    short one = 1;

                    fout.write((char*)&edgeIT->start, sizeof(unsigned));
                    fout.write((char*)&edgeIT->target, sizeof(unsigned));
                    fout.write((char*)&intDist, sizeof(int));
                    switch(edgeIT->direction) {
                    case _Way::notSure:
                        fout.write((char*)&zero, sizeof(short));
                        break;
                    case _Way::oneway:
                        fout.write((char*)&one, sizeof(short));
                        break;
                    case _Way::bidirectional:
                        fout.write((char*)&zero, sizeof(short));

                        break;
                    case _Way::opposite:
                        fout.write((char*)&one, sizeof(short));
                        break;
                    default:
                        std::cerr << "[error] edge with no direction: " << edgeIT->direction << std::endl;
                        assert(false);
                        break;
                    }
                    fout.write((char*)&intWeight, sizeof(int));
                    short edgeType = edgeIT->type;
                    fout.write((char*)&edgeType, sizeof(short));
                    fout.write((char*)&edgeIT->nameID, sizeof(unsigned));
                }
                usedEdgeCounter++;
                edgeIT++;
            }
        }
        std::cout << "ok, after " << get_timestamp() - time << "s" << std::endl;
        time = get_timestamp();

        std::cout << "[extractor] setting number of edges   ... " << std::flush;
        fout.seekp(positionInFile);
        fout.write((char*)&usedEdgeCounter, sizeof(unsigned));
        fout.close();
        std::cout << "ok" << std::endl;
        time = get_timestamp();


        std::cout << "[extractor] writing street name index ... " << std::flush;
        std::vector<unsigned> * nameIndex = new std::vector<unsigned>(nameVector.size()+1, 0);
        outputFileName.append(".names");
        std::ofstream nameOutFile(outputFileName.c_str(), std::ios::binary);
        unsigned sizeOfNameIndex = nameIndex->size();
        nameOutFile.write((char *)&(sizeOfNameIndex), sizeof(unsigned));

        for(STXXLStringVector::iterator it = nameVector.begin(); it != nameVector.end(); it++) {
            unsigned lengthOfRawString = strlen(it->c_str());
            nameOutFile.write((char *)&(lengthOfRawString), sizeof(unsigned));
            nameOutFile.write(it->c_str(), lengthOfRawString);
        }

        nameOutFile.close();
        delete nameIndex;
        std::cout << "ok, after " << get_timestamp() - time << "s" << std::endl;

        //        time = get_timestamp();
        //        std::cout << "[extractor] writing address list      ... " << std::flush;
        //
        //        adressFileName.append(".address");
        //        std::ofstream addressOutFile(adressFileName.c_str());
        //        for(STXXLAddressVector::iterator it = adressVector.begin(); it != adressVector.end(); it++) {
        //            addressOutFile << it->node.id << "|" << it->node.lat << "|" << it->node.lon << "|" << it->city << "|" << it->street << "|" << it->housenumber << "|" << it->state << "|" << it->country << "\n";
        //        }
        //        addressOutFile.close();
        //        std::cout << "ok, after " << get_timestamp() - time << "s" << std::endl;

    } catch ( const std::exception& e ) {
        std::cerr <<  "Caught Execption:" << e.what() << std::endl;
        return false;
    }

    delete extractCallBacks;
    std::cout << "[extractor] finished." << std::endl;
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

