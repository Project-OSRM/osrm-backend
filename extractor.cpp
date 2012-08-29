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

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}
#include <luabind/luabind.hpp>

#include <libxml/xmlreader.h>
#include <boost/foreach.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <unistd.h>

#include "typedefs.h"
#include "DataStructures/InputReaderFactory.h"
#include "Extractor/ExtractorCallbacks.h"
#include "Extractor/ExtractorStructs.h"
#include "Extractor/LuaUtil.h"
#include "Extractor/PBFParser.h"
#include "Extractor/XMLParser.h"
#include "Util/BaseConfiguration.h"
#include "Util/InputFileUtil.h"
#include "Util/MachineInfo.h"
#include "Util/StringUtil.h"

#include "Extractor/ExtractionContainers.h"

using namespace std;

typedef BaseConfiguration ExtractorConfiguration;

unsigned globalRestrictionCounter = 0;
ExtractorCallbacks * extractCallBacks;
//
bool nodeFunction(_Node n);
bool restrictionFunction(_RawRestrictionContainer r);
bool wayFunction(_Way w);

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
            } else if(name == "accessTags") {
                std::vector<std::string> tokens;
                stringSplit(value, ',', tokens);
                settings.accessTags = tokens;
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
            } else if( name == "accessForbiddenKeys") {
                //split value at commas
                std::vector<std::string> tokens;
                stringSplit(value, ',', tokens);
                //put each value into map
                BOOST_FOREACH(std::string & s, tokens) {
                    INFO("adding " << s << " to accessForbiddenKeys");
                    settings.accessForbiddenKeys.insert(std::make_pair(s, true));
                }
            } else if( name == "accessForbiddenDefault") {
                //split value at commas
                std::vector<std::string> tokens;
                stringSplit(value, ',', tokens);
                //put each value into map
                BOOST_FOREACH(std::string & s, tokens) {
                    INFO("adding " << s << " to accessForbiddenDefault");
                    settings.accessForbiddenDefault.insert(std::make_pair(s, true));
                }
            }
            settings.speedProfile[name] = std::make_pair(std::atoi(value.c_str()), settings.speedProfile.size() );
        }
    } catch(std::exception& e) {
        ERR("caught: " << e.what() );
    }

    /*** Setup Scripting Environment ***/

    // Create a new lua state
    lua_State *myLuaState = luaL_newstate();

    // Connect LuaBind to this lua state
    luabind::open(myLuaState);

    // Add our function to the state's global scope
    luabind::module(myLuaState) [
      luabind::def("print", LUA_print<std::string>)
    ];

    if(0 != luaL_dostring(
      myLuaState,
      "print('Initializing LUA engine')\n"
    )) {
        ERR(lua_tostring(myLuaState,-1)<< " occured in scripting block");
    }

    luabind::module(myLuaState) [
      luabind::class_<HashTable<std::string, std::string> >("keyVals")
      .def("Add", &HashTable<std::string, std::string>::Add)
      .def("Find", &HashTable<std::string, std::string>::Find)
    ];

    luabind::module(myLuaState) [
      luabind::class_<ImportNode>("Node")
          .def(luabind::constructor<>())
          .def_readwrite("lat", &ImportNode::lat)
          .def_readwrite("lon", &ImportNode::lon)
          .def_readwrite("id", &ImportNode::id)
          .def_readwrite("bollard", &ImportNode::bollard)
          .def_readwrite("traffic_light", &ImportNode::trafficLight)
          .def_readwrite("tags", &ImportNode::keyVals)
    ];
    // Now call our function in a lua script
    if(0 != luaL_dofile(myLuaState, "profile.lua")) {
        ERR(lua_tostring(myLuaState,-1)<< " occured in scripting block");
    }

    //open string library;
    luaopen_string(myLuaState);

    /*** End of Scripting Environment Setup; ***/

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
    ExtractionContainers externalMemory;

    stringMap[""] = 0;
    extractCallBacks = new ExtractorCallbacks(&externalMemory, settings, &stringMap);
    BaseParser<_Node, _RawRestrictionContainer, _Way> * parser;
    if(isPBF) {
        parser = new PBFParser(argv[1]);
    } else {
        parser = new XMLParser(argv[1]);
    }
    parser->RegisterCallbacks(&nodeFunction, &restrictionFunction, &wayFunction);
    parser->RegisterLUAState(myLuaState);

    if(!parser->Init())
        INFO("Parser not initialized!");
    parser->Parse();

    externalMemory.PrepareData(settings, outputFileName, restrictionsFileName, amountOfRAM);

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

bool restrictionFunction(_RawRestrictionContainer r) {
    extractCallBacks->restrictionFunction(r);
    ++globalRestrictionCounter;
    return true;
}
bool wayFunction(_Way w) {
    extractCallBacks->wayFunction(w);
    return true;
}
