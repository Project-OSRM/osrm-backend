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
#include <exception>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <libxml/xmlreader.h>
#include <boost/foreach.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
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
#include "Util/StringUtil.h"

using namespace std;

typedef BaseConfiguration ExtractorConfiguration;

unsigned globalRestrictionCounter = 0;
ExtractorCallbacks * extractCallBacks;

bool nodeFunction(_Node n);
bool adressFunction(_Node n, HashTable<string, string> & keyVals);
bool restrictionFunction(_RawRestrictionContainer r);
bool wayFunction(_Way w);

template<class ClassT>
bool removeIfUnused(ClassT n) { return (false == n.used); }


int main (int argc, char *argv[]) {

    if(argc < 2) {
        ERR("usage: \n" << argv[0] << " <file.osm/.osm.bz2/.osm.pbf>");
    }

    //Check if another instance of stxxl is already running or if there is a general problem
    try {
        stxxl::vector<unsigned> testForRunningInstance;
    } catch(std::exception & e) {
        ERR("Could not instantiate STXXL layer." << std::endl << e.what());
    }
    double startupTime = get_timestamp();

    INFO("extracting data from input file " << argv[1]);
    bool isPBF(false);
    std::string outputFileName(argv[1]);
    std::string restrictionsFileName(argv[1]);
    std::string::size_type pos = outputFileName.find(".osm.bz2");
    if(pos==std::string::npos) {
        pos = outputFileName.find(".osm.pbf");
        if(pos!=std::string::npos) {
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
    std::string adressFileName(outputFileName);
    Settings settings;

    boost::property_tree::ptree pt;
    try {
        INFO("Loading speed profiles");
        boost::property_tree::ini_parser::read_ini("speedprofile.ini", pt);
        INFO("Found the following speed profiles: ");
        int profileCounter(0);
        BOOST_FOREACH(boost::property_tree::ptree::value_type &v, pt.get_child("")) {
            std::string name = v.first;
            cout << " [" << profileCounter << "]" << name << endl;
            ++profileCounter;
        }
        std::string usedSpeedProfile(pt.get_child("").begin()->first);
        INFO("Using profile \"" << usedSpeedProfile << "\"")
        BOOST_FOREACH(boost::property_tree::ptree::value_type &v, pt.get_child(usedSpeedProfile)) {
            std::string name = v.first;
            std::string value = v.second.get<std::string>("");
            DEBUG("inserting " << name << "=" << value);
            if(name == "obeyOneways") {
                if(value == "no")
                    settings.obeyOneways = false;
            } else if(name == "obeyBollards") {
                if(value == "no") {
                    settings.obeyBollards = false;
                }
            } else if(name == "useRestrictions") {
                if(value == "no")
                    settings.useRestrictions = false;
            } else if(name == "accessTag") {
                settings.accessTag = value;
            } else if(name == "excludeFromGrid") {
                settings.excludeFromGrid = value;
            } else if(name == "defaultSpeed") {
                settings.defaultSpeed = atoi(value.c_str());
                settings.speedProfile["default"] = std::make_pair(settings.defaultSpeed, settings.speedProfile.size() );
            } else if( name == "takeMinimumOfSpeeds") {
                settings.takeMinimumOfSpeeds = ("yes" == value);
            } else if( name == "ignoreAreas") {
                settings.ignoreAreas = ("yes" == value);
            } else if( name == "accessRestrictedService") {
                //split value at commas
                std::vector<std::string> tokens;
                stringSplit(value, ',', tokens);
                //put each value into map
                BOOST_FOREACH(std::string & s, tokens) {
                    INFO("adding " << s << " to accessRestrictedService");
                    settings.accessRestrictedService.insert(std::make_pair(s, true));
                }
            } else if( name == "accessRestrictionKeys") {
                //split value at commas
                std::vector<std::string> tokens;
                stringSplit(value, ',', tokens);
                //put each value into map
                BOOST_FOREACH(std::string & s, tokens) {
                    INFO("adding " << s << " to accessRestrictionKeys");
                    settings.accessRestrictionKeys.insert(std::make_pair(s, true));
                }
            }
            settings.speedProfile[name] = std::make_pair(std::atoi(value.c_str()), settings.speedProfile.size() );
        }
    } catch(std::exception& e) {
        ERR("caught: " << e.what() );
    }

    unsigned amountOfRAM = 1;
    unsigned installedRAM = GetPhysicalmemory(); 
    if(installedRAM < 2048264) {
        WARN("Machine has less than 2GB RAM.");
    }
    if(testDataFile("extractor.ini")) {
        ExtractorConfiguration extractorConfig("extractor.ini");
        unsigned memoryAmountFromFile = atoi(extractorConfig.GetParameter("Memory").c_str());
        if( memoryAmountFromFile != 0 && memoryAmountFromFile <= installedRAM/(1024*1024))
            amountOfRAM = memoryAmountFromFile;
        INFO("Using " << amountOfRAM << " GB of RAM for buffers");
    }

    StringMap stringMap;
    STXXLContainers externalMemory;

    unsigned usedNodeCounter = 0;
    unsigned usedEdgeCounter = 0;
    double time = get_timestamp();

    stringMap[""] = 0;
    extractCallBacks = new ExtractorCallbacks(&externalMemory, settings, &stringMap);
    BaseParser<_Node, _RawRestrictionContainer, _Way> * parser;
    if(isPBF) {
        parser = new PBFParser(argv[1]);
    } else {
        parser = new XMLParser(argv[1]);
    }
    parser->RegisterCallbacks(&nodeFunction, &restrictionFunction, &wayFunction, &adressFunction);
    if(!parser->Init())
        INFO("Parser not initialized!");
    parser->Parse();

    try {
        //        INFO("raw no. of names:        " << externalMemory.nameVector.size());
        //        INFO("raw no. of nodes:        " << externalMemory.allNodes.size());
        //        INFO("no. of used nodes:       " << externalMemory.usedNodeIDs.size());
        //        INFO("raw no. of edges:        " << externalMemory.allEdges.size());
        //        INFO("raw no. of ways:         " << externalMemory.wayStartEndVector.size());
        //        INFO("raw no. of addresses:    " << externalMemory.adressVector.size());
        //        INFO("raw no. of restrictions: " << externalMemory.restrictionsVector.size());

        cout << "[extractor] parsing finished after " << get_timestamp() - time << " seconds" << endl;
        time = get_timestamp();
        boost::uint64_t memory_to_use = static_cast<boost::uint64_t>(amountOfRAM) * 1024 * 1024 * 1024;

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
        stxxl::sort(externalMemory.wayStartEndVector.begin(), externalMemory.wayStartEndVector.end(), CmpWayByID(), memory_to_use);
        cout << "ok, after " << get_timestamp() - time << "s" << endl;

        cout << "[extractor] Sorting restrctns. by from... " << flush;
        stxxl::sort(externalMemory.restrictionsVector.begin(), externalMemory.restrictionsVector.end(), CmpRestrictionContainerByFrom(), memory_to_use);
        cout << "ok, after " << get_timestamp() - time << "s" << endl;

        cout << "[extractor] Fixing restriction starts ... " << flush;
        STXXLRestrictionsVector::iterator restrictionsIT = externalMemory.restrictionsVector.begin();
        STXXLWayIDStartEndVector::iterator wayStartAndEndEdgeIT = externalMemory.wayStartEndVector.begin();

        while(wayStartAndEndEdgeIT != externalMemory.wayStartEndVector.end() && restrictionsIT != externalMemory.restrictionsVector.end()) {
            if(wayStartAndEndEdgeIT->wayID < restrictionsIT->fromWay){
                ++wayStartAndEndEdgeIT;
                continue;
            }
            if(wayStartAndEndEdgeIT->wayID > restrictionsIT->fromWay) {
                ++restrictionsIT;
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
            ++restrictionsIT;
        }

        cout << "ok, after " << get_timestamp() - time << "s" << endl;
        time = get_timestamp();

        cout << "[extractor] Sorting restrctns. by to  ... " << flush;
        stxxl::sort(externalMemory.restrictionsVector.begin(), externalMemory.restrictionsVector.end(), CmpRestrictionContainerByTo(), memory_to_use);
        cout << "ok, after " << get_timestamp() - time << "s" << endl;

        time = get_timestamp();
        unsigned usableRestrictionsCounter(0);
        cout << "[extractor] Fixing restriction ends   ... " << flush;
        restrictionsIT = externalMemory.restrictionsVector.begin();
        wayStartAndEndEdgeIT = externalMemory.wayStartEndVector.begin();
        while(wayStartAndEndEdgeIT != externalMemory.wayStartEndVector.end() && restrictionsIT != externalMemory.restrictionsVector.end()) {
            if(wayStartAndEndEdgeIT->wayID < restrictionsIT->toWay){
                ++wayStartAndEndEdgeIT;
                continue;
            }
            if(wayStartAndEndEdgeIT->wayID > restrictionsIT->toWay) {
                ++restrictionsIT;
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
                ++usableRestrictionsCounter;
            }
            ++restrictionsIT;
        }
        cout << "ok, after " << get_timestamp() - time << "s" << endl;
        INFO("usable restrictions: " << usableRestrictionsCounter );
        //serialize restrictions
        ofstream restrictionsOutstream;
        restrictionsOutstream.open(restrictionsFileName.c_str(), ios::binary);
        restrictionsOutstream.write((char*)&usableRestrictionsCounter, sizeof(unsigned));
        for(restrictionsIT = externalMemory.restrictionsVector.begin(); restrictionsIT != externalMemory.restrictionsVector.end(); ++restrictionsIT) {
            if(UINT_MAX != restrictionsIT->restriction.fromNode && UINT_MAX != restrictionsIT->restriction.toNode) {
                restrictionsOutstream.write((char *)&(restrictionsIT->restriction), sizeof(_Restriction));
            }
        }
        restrictionsOutstream.close();

        ofstream fout;
        fout.open(outputFileName.c_str(), ios::binary);
        fout.write((char*)&usedNodeCounter, sizeof(unsigned));
        time = get_timestamp();
        cout << "[extractor] Confirming/Writing used nodes     ... " << flush;

        STXXLNodeVector::iterator nodesIT = externalMemory.allNodes.begin();
        STXXLNodeIDVector::iterator usedNodeIDsIT = externalMemory.usedNodeIDs.begin();
        while(usedNodeIDsIT != externalMemory.usedNodeIDs.end() && nodesIT != externalMemory.allNodes.end()) {
            if(*usedNodeIDsIT < nodesIT->id){
                ++usedNodeIDsIT;
                continue;
            }
            if(*usedNodeIDsIT > nodesIT->id) {
                ++nodesIT;
                continue;
            }
            if(*usedNodeIDsIT == nodesIT->id) {
                if(!settings.obeyBollards && nodesIT->bollard)
                    nodesIT->bollard = false;
                fout.write((char*)&(*nodesIT), sizeof(_Node));
                ++usedNodeCounter;
                ++usedNodeIDsIT;
                ++nodesIT;
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
                ++edgeIT;
                continue;
            }
            if(edgeIT->start > nodesIT->id) {
                nodesIT++;
                continue;
            }
            if(edgeIT->start == nodesIT->id) {
                edgeIT->startCoord.lat = nodesIT->lat;
                edgeIT->startCoord.lon = nodesIT->lon;
                ++edgeIT;
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
                ++edgeIT;
                continue;
            }
            if(edgeIT->target > nodesIT->id) {
                ++nodesIT;
                continue;
            }
            if(edgeIT->target == nodesIT->id) {
                if(edgeIT->startCoord.lat != INT_MIN && edgeIT->startCoord.lon != INT_MIN) {
                    edgeIT->targetCoord.lat = nodesIT->lat;
                    edgeIT->targetCoord.lon = nodesIT->lon;

                    double distance = ApproximateDistance(edgeIT->startCoord.lat, edgeIT->startCoord.lon, nodesIT->lat, nodesIT->lon);
                    assert(edgeIT->speed != -1);
                    double weight = ( distance * 10. ) / (edgeIT->speed / 3.6);
                    int intWeight = std::max(1, (int)(edgeIT->isDurationSet ? edgeIT->speed : weight) );
                    int intDist = std::max(1, (int)distance);
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
                    assert(edgeIT->type >= 0);
                    fout.write((char*)&edgeIT->type, sizeof(short));
                    fout.write((char*)&edgeIT->nameID, sizeof(unsigned));
                    fout.write((char*)&edgeIT->isRoundabout, sizeof(bool));
                    fout.write((char*)&edgeIT->ignoreInGrid, sizeof(bool));
                    fout.write((char*)&edgeIT->isAccessRestricted, sizeof(bool));
                }
                ++usedEdgeCounter;
                ++edgeIT;
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
        std::string nameOutFileName = (outputFileName + ".names");
        ofstream nameOutFile(nameOutFileName.c_str(), ios::binary);
        unsigned sizeOfNameIndex = externalMemory.nameVector.size();
        nameOutFile.write((char *)&(sizeOfNameIndex), sizeof(unsigned));

        BOOST_FOREACH(string str, externalMemory.nameVector) {
            unsigned lengthOfRawString = strlen(str.c_str());
            nameOutFile.write((char *)&(lengthOfRawString), sizeof(unsigned));
            nameOutFile.write(str.c_str(), lengthOfRawString);
        }

        nameOutFile.close();
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

    double endTime = (get_timestamp() - startupTime);
    INFO("Processed " << (usedNodeCounter)/(endTime) << " nodes/sec and " << usedEdgeCounter/endTime << " edges/sec");
    stringMap.clear();
    delete parser;
    delete extractCallBacks;
    INFO("[extractor] finished.");
    std::cout << "\nRun:\n"
                   "./osrm-prepare " << outputFileName << " " << restrictionsFileName << std::endl;
    return 0;
}

bool nodeFunction(_Node n) {
    extractCallBacks->nodeFunction(n);
    return true;
}

bool adressFunction(_Node n, HashTable<string, string> & keyVals){
    extractCallBacks->adressFunction(n, keyVals);
    return true;
}

bool restrictionFunction(_RawRestrictionContainer r) {
    extractCallBacks->restrictionFunction(r);
    ++globalRestrictionCounter;
    return true;
}
bool wayFunction(_Way w) {
    extractCallBacks->wayFunction(w);
    return true;
}
