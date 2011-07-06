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
#include "Util/MachineInfo.h"

typedef BaseConfiguration ExtractorConfiguration;

unsigned globalRestrictionCounter = 0;
ExtractorCallbacks * extractCallBacks;

bool nodeFunction(_Node n);
bool adressFunction(_Node n, HashTable<string, string> keyVals);
bool restrictionFunction(_RawRestrictionContainer r);
bool wayFunction(_Way w);

template<class ClassT>
bool removeIfUnused(ClassT n) { return (false == n.used); }

int main (int argc, char *argv[]) {
    if(argc <= 1) {
        cerr << "usage: " << endl << argv[0] << " <file.osm/.osm.bz2/.osm.pbf>" << endl;
        exit(-1);
    }

    cout << "[extractor] extracting data from input file " << argv[1] << endl;
    bool isPBF = false;
    string outputFileName(argv[1]);
    string restrictionsFileName(argv[1]);
    string::size_type pos = outputFileName.find(".osm.bz2");
    if(pos==string::npos) {
        pos = outputFileName.find(".osm.pbf");
        if(pos!=string::npos) {
            isPBF = true;
        }
    }
    if(pos!=string::npos) {
        outputFileName.replace(pos, 8, ".osrm");
        restrictionsFileName.replace(pos, 8, ".osrm.restrictions");
    } else {
        pos=outputFileName.find(".osm");
        if(pos!=string::npos) {
            outputFileName.replace(pos, 5, ".osrm");
            restrictionsFileName.replace(pos, 5, ".osrm.restrictions");
        } else {
            outputFileName.append(".osrm");
            restrictionsFileName.append(".osrm.restrictions");
        }
    }
    string adressFileName(outputFileName);

    unsigned amountOfRAM = 1;
    unsigned installedRAM = GetPhysicalmemory(); 
    if(installedRAM < 2048264) {
        cout << "[Warning] Machine has less than 2GB RAM." << endl;
    }
    if(testDataFile("extractor.ini")) {
        ExtractorConfiguration extractorConfig("extractor.ini");
        unsigned memoryAmountFromFile = atoi(extractorConfig.GetParameter("Memory").c_str());
        if( memoryAmountFromFile != 0 && memoryAmountFromFile <= installedRAM/(1024*1024*1024))
            amountOfRAM = memoryAmountFromFile;
        cout << "[extractor] using " << amountOfRAM << " GB of RAM for buffers" << endl;
    }

    STXXLContainers externalMemory;

    unsigned usedNodeCounter = 0;
    unsigned usedEdgeCounter = 0;

    StringMap * stringMap = new StringMap();
    Settings settings;
    settings.speedProfile.names.insert(settings.speedProfile.names.begin(), names, names+14);
    settings.speedProfile.speed.insert(settings.speedProfile.speed.begin(), speeds, speeds+14);

    double time = get_timestamp();

    stringMap->set_empty_key(GetRandomString());
    stringMap->insert(make_pair("", 0));
    extractCallBacks = new ExtractorCallbacks(&externalMemory, settings, stringMap);

    BaseParser<_Node, _RawRestrictionContainer, _Way> * parser;
    if(isPBF) {
        parser = new PBFParser(argv[1]);
    } else {
        parser = new XMLParser(argv[1]);
    }
    parser->RegisterCallbacks(&nodeFunction, &restrictionFunction, &wayFunction, &adressFunction);
    if(parser->Init()) {
        parser->Parse();
    } else {
        cerr << "[error] parser not initialized!" << endl;
        exit(-1);
    }
    delete parser;

    try {
//        INFO("raw no. of names:        " << externalMemory.nameVector.size());
//        INFO("raw no. of nodes:        " << externalMemory.allNodes.size());
//        INFO("no. of used nodes:       " << externalMemory.usedNodeIDs.size());
//        INFO("raw no. of edges:        " << externalMemory.allEdges.size());
//        INFO("raw no. of ways:         " << externalMemory.wayStartEndVector.size());
//        INFO("raw no. of addresses:    " << externalMemory.adressVector.size());
//        INFO("raw no. of restrictions: " << externalMemory.restrictionsVector.size());

        cout << "[extractor] parsing finished after " << get_timestamp() - time << "seconds" << endl;
        time = get_timestamp();
        unsigned memory_to_use = amountOfRAM * 1024 * 1024 * 1024;

        cout << "[extractor] Sorting used nodes        ... " << flush;
        stxxl::sort(externalMemory.usedNodeIDs.begin(), externalMemory.usedNodeIDs.end(), Cmp(), memory_to_use);
        cout << "ok, after " << get_timestamp() - time << "s" << endl;

        time = get_timestamp();
        cout << "[extractor] Erasing duplicate nodes   ... " << flush;
        stxxl::vector<NodeID>::iterator NewEnd = unique ( externalMemory.usedNodeIDs.begin(),externalMemory.usedNodeIDs.end() ) ;
        externalMemory.usedNodeIDs.resize ( NewEnd - externalMemory.usedNodeIDs.begin() );
        cout << "ok, after " << get_timestamp() - time << "s" << endl;
        time = get_timestamp();

        cout << "[extractor] Sorting all nodes         ... " << flush;
        stxxl::sort(externalMemory.allNodes.begin(), externalMemory.allNodes.end(), CmpNodeByID(), memory_to_use);
        cout << "ok, after " << get_timestamp() - time << "s" << endl;
        time = get_timestamp();

        cout << "[extractor] Sorting used ways         ... " << flush;
        stxxl::sort(externalMemory.wayStartEndVector.begin(), externalMemory.wayStartEndVector.end(), CmpWayStartAndEnd(), memory_to_use);
        cout << "ok, after " << get_timestamp() - time << "s" << endl;

        cout << "[extractor] Sorting restrctns. by from... " << flush;
        stxxl::sort(externalMemory.restrictionsVector.begin(), externalMemory.restrictionsVector.end(), CmpRestrictionByFrom(), memory_to_use);
        cout << "ok, after " << get_timestamp() - time << "s" << endl;

        cout << "[extractor] Fixing restriction starts ... " << flush;
        STXXLRestrictionsVector::iterator restrictionsIT = externalMemory.restrictionsVector.begin();
        STXXLWayIDStartEndVector::iterator wayStartAndEndEdgeIT = externalMemory.wayStartEndVector.begin();

        while(wayStartAndEndEdgeIT != externalMemory.wayStartEndVector.end() && restrictionsIT != externalMemory.restrictionsVector.end()) {
            if(wayStartAndEndEdgeIT->wayID < restrictionsIT->fromWay){
                wayStartAndEndEdgeIT++;
                continue;
            }
            if(wayStartAndEndEdgeIT->wayID > restrictionsIT->fromWay) {
                restrictionsIT++;
                continue;
            }
            assert(wayStartAndEndEdgeIT->wayID == restrictionsIT->fromWay);
            NodeID viaNode = restrictionsIT->restriction.viaNode;

            if(wayStartAndEndEdgeIT->firstStart == viaNode) {
                restrictionsIT->restriction.fromNode = wayStartAndEndEdgeIT->firstTarget;
            } else if(wayStartAndEndEdgeIT->firstTarget == viaNode) {
                restrictionsIT->restriction.fromNode = wayStartAndEndEdgeIT->firstStart;
            } else if(wayStartAndEndEdgeIT->lastStart == viaNode) {
                restrictionsIT->restriction.fromNode = wayStartAndEndEdgeIT->lastTarget;
            } else if(wayStartAndEndEdgeIT->lastTarget == viaNode) {
                restrictionsIT->restriction.fromNode = wayStartAndEndEdgeIT->lastStart;
            }
            restrictionsIT++;
        }

        cout << "ok, after " << get_timestamp() - time << "s" << endl;
        time = get_timestamp();

        cout << "[extractor] Sorting restrctns. by to  ... " << flush;
        stxxl::sort(externalMemory.restrictionsVector.begin(), externalMemory.restrictionsVector.end(), CmpRestrictionByTo(), memory_to_use);
        cout << "ok, after " << get_timestamp() - time << "s" << endl;

        time = get_timestamp();
        unsigned usableRestrictionsCounter(0);
        cout << "[extractor] Fixing restriction ends   ... " << flush;
        restrictionsIT = externalMemory.restrictionsVector.begin();
        wayStartAndEndEdgeIT = externalMemory.wayStartEndVector.begin();
        while(wayStartAndEndEdgeIT != externalMemory.wayStartEndVector.end() &&
                restrictionsIT != externalMemory.restrictionsVector.end()) {
            if(wayStartAndEndEdgeIT->wayID < restrictionsIT->toWay){
                wayStartAndEndEdgeIT++;
                continue;
            }
            if(wayStartAndEndEdgeIT->wayID > restrictionsIT->toWay) {
                restrictionsIT++;
                continue;
            }
            NodeID viaNode = restrictionsIT->restriction.viaNode;
            if(wayStartAndEndEdgeIT->lastStart == viaNode) {
                restrictionsIT->restriction.toNode = wayStartAndEndEdgeIT->lastTarget;
            } else if(wayStartAndEndEdgeIT->lastTarget == viaNode) {
                restrictionsIT->restriction.toNode = wayStartAndEndEdgeIT->lastStart;
            } else if(wayStartAndEndEdgeIT->firstStart == viaNode) {
                restrictionsIT->restriction.toNode = wayStartAndEndEdgeIT->firstTarget;
            } else if(wayStartAndEndEdgeIT->firstTarget == viaNode) {
                restrictionsIT->restriction.toNode = wayStartAndEndEdgeIT->firstStart;
            }

            if(UINT_MAX != restrictionsIT->restriction.fromNode && UINT_MAX != restrictionsIT->restriction.toNode) {
                usableRestrictionsCounter++;
            }
            restrictionsIT++;
        }

        cout << "ok, after " << get_timestamp() - time << "s" << endl;
        //todo: serialize restrictions
        ofstream restrictionsOutstream;
        restrictionsOutstream.open(restrictionsFileName.c_str(), ios::binary);
        restrictionsOutstream.write((char*)&usableRestrictionsCounter, sizeof(unsigned));
        for(restrictionsIT = externalMemory.restrictionsVector.begin(); restrictionsIT != externalMemory.restrictionsVector.end(); restrictionsIT++) {
            if(UINT_MAX != restrictionsIT->restriction.fromNode && UINT_MAX != restrictionsIT->restriction.toNode) {
                restrictionsOutstream.write((char *)&(restrictionsIT->restriction), sizeof(_Restriction));
            }
        }
        restrictionsOutstream.close();

        ofstream fout;
        fout.open(outputFileName.c_str(), ios::binary);
        fout.write((char*)&usedNodeCounter, sizeof(unsigned));
        time = get_timestamp();
        cout << "[extractor] Confirming used nodes     ... " << flush;
        STXXLNodeVector::iterator nodesIT = externalMemory.allNodes.begin();
        STXXLNodeIDVector::iterator usedNodeIDsIT = externalMemory.usedNodeIDs.begin();
        while(usedNodeIDsIT != externalMemory.usedNodeIDs.end() && nodesIT != externalMemory.allNodes.end()) {
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

        cout << "ok, after " << get_timestamp() - time << "s" << endl;
        time = get_timestamp();

        cout << "[extractor] setting number of nodes   ... " << flush;
        ios::pos_type positionInFile = fout.tellp();
        fout.seekp(ios::beg);
        fout.write((char*)&usedNodeCounter, sizeof(unsigned));
        fout.seekp(positionInFile);

        cout << "ok" << endl;
        time = get_timestamp();

        // Sort edges by start.
        cout << "[extractor] Sorting edges by start    ... " << flush;
        stxxl::sort(externalMemory.allEdges.begin(), externalMemory.allEdges.end(), CmpEdgeByStartID(), memory_to_use);
        cout << "ok, after " << get_timestamp() - time << "s" << endl;
        time = get_timestamp();

        cout << "[extractor] Setting start coords      ... " << flush;
        fout.write((char*)&usedEdgeCounter, sizeof(unsigned));
        // Traverse list of edges and nodes in parallel and set start coord
        nodesIT = externalMemory.allNodes.begin();
        STXXLEdgeVector::iterator edgeIT = externalMemory.allEdges.begin();
        while(edgeIT != externalMemory.allEdges.end() && nodesIT != externalMemory.allNodes.end()) {
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
        cout << "ok, after " << get_timestamp() - time << "s" << endl;
        time = get_timestamp();

        // Sort Edges by target
        cout << "[extractor] Sorting edges by target   ... " << flush;
        stxxl::sort(externalMemory.allEdges.begin(), externalMemory.allEdges.end(), CmpEdgeByTargetID(), memory_to_use);
        cout << "ok, after " << get_timestamp() - time << "s" << endl;
        time = get_timestamp();

        cout << "[extractor] Setting target coords     ... " << flush;
        // Traverse list of edges and nodes in parallel and set target coord
        nodesIT = externalMemory.allNodes.begin();
        edgeIT = externalMemory.allEdges.begin();
        while(edgeIT != externalMemory.allEdges.end() && nodesIT != externalMemory.allNodes.end()) {
            if(edgeIT->target < nodesIT->id){
                edgeIT++;
                continue;
            }
            if(edgeIT->target > nodesIT->id) {
                nodesIT++;
                continue;
            }
            if(edgeIT->target == nodesIT->id) {
                if(edgeIT->startCoord.lat != INT_MIN && edgeIT->startCoord.lon != INT_MIN) {
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
                        cerr << "[error] edge with no direction: " << edgeIT->direction << endl;
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
        cout << "ok, after " << get_timestamp() - time << "s" << endl;
        time = get_timestamp();

        cout << "[extractor] setting number of edges   ... " << flush;
        fout.seekp(positionInFile);
        fout.write((char*)&usedEdgeCounter, sizeof(unsigned));
        fout.close();
        cout << "ok" << endl;
        time = get_timestamp();


        cout << "[extractor] writing street name index ... " << flush;
        vector<unsigned> * nameIndex = new vector<unsigned>(externalMemory.nameVector.size()+1, 0);
        outputFileName.append(".names");
        ofstream nameOutFile(outputFileName.c_str(), ios::binary);
        unsigned sizeOfNameIndex = nameIndex->size();
        nameOutFile.write((char *)&(sizeOfNameIndex), sizeof(unsigned));

        for(STXXLStringVector::iterator it = externalMemory.nameVector.begin(); it != externalMemory.nameVector.end(); it++) {
            unsigned lengthOfRawString = strlen(it->c_str());
            nameOutFile.write((char *)&(lengthOfRawString), sizeof(unsigned));
            nameOutFile.write(it->c_str(), lengthOfRawString);
        }

        nameOutFile.close();
        delete nameIndex;
        cout << "ok, after " << get_timestamp() - time << "s" << endl;

        //        time = get_timestamp();
        //        cout << "[extractor] writing address list      ... " << flush;
        //
        //        adressFileName.append(".address");
        //        ofstream addressOutFile(adressFileName.c_str());
        //        for(STXXLAddressVector::iterator it = adressVector.begin(); it != adressVector.end(); it++) {
        //            addressOutFile << it->node.id << "|" << it->node.lat << "|" << it->node.lon << "|" << it->city << "|" << it->street << "|" << it->housenumber << "|" << it->state << "|" << it->country << "\n";
        //        }
        //        addressOutFile.close();
        //        cout << "ok, after " << get_timestamp() - time << "s" << endl;

    } catch ( const exception& e ) {
        cerr <<  "Caught Execption:" << e.what() << endl;
        return false;
    }

    delete extractCallBacks;
    cout << "[extractor] finished." << endl;
    return 0;
}

bool nodeFunction(_Node n) {
    extractCallBacks->nodeFunction(n);
    return true;
}

bool adressFunction(_Node n, HashTable<string, string> keyVals){
    extractCallBacks->adressFunction(n, keyVals);
    return true;
}

bool restrictionFunction(_RawRestrictionContainer r) {
    extractCallBacks->restrictionFunction(r);
    globalRestrictionCounter++;
    return true;
}
bool wayFunction(_Way w) {
    extractCallBacks->wayFunction(w);
    return true;
}

