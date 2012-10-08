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

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <string>

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}
#include <luabind/luabind.hpp>

#include "typedefs.h"
#include "Extractor/ExtractorCallbacks.h"
#include "Extractor/ExtractionContainers.h"
#include "Extractor/ExtractionHelperFunctions.h"
#include "Extractor/ExtractorStructs.h"
#include "Extractor/LuaUtil.h"
#include "Extractor/PBFParser.h"
#include "Extractor/XMLParser.h"
#include "Util/BaseConfiguration.h"
#include "Util/InputFileUtil.h"
#include "Util/MachineInfo.h"

typedef BaseConfiguration ExtractorConfiguration;

ExtractorCallbacks * extractCallBacks;
//
bool nodeFunction(_Node n);
bool restrictionFunction(_RawRestrictionContainer r);
bool wayFunction(_Way w);

int main (int argc, char *argv[]) {
    if(argc < 2) {
        ERR("usage: \n" << argv[0] << " <file.osm/.osm.bz2/.osm.pbf> [<profile.lua>]");
    }

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

    /*** Setup Scripting Environment ***/

    // Create a new lua state
    lua_State *myLuaState = luaL_newstate();

    // Connect LuaBind to this lua state
    luabind::open(myLuaState);

    // Add our function to the state's global scope
    luabind::module(myLuaState) [
      luabind::def("print", LUA_print<std::string>),
      luabind::def("parseMaxspeed", parseMaxspeed),
      luabind::def("durationIsValid", durationIsValid),
      luabind::def("parseDuration", parseDuration)
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

    luabind::module(myLuaState) [
      luabind::class_<_Way>("Way")
          .def(luabind::constructor<>())
          .def_readwrite("name", &_Way::name)
          .def_readwrite("speed", &_Way::speed)
          .def_readwrite("type", &_Way::type)
          .def_readwrite("access", &_Way::access)
          .def_readwrite("roundabout", &_Way::roundabout)
          .def_readwrite("is_duration_set", &_Way::isDurationSet)
          .def_readwrite("is_access_restricted", &_Way::isAccessRestricted)
          .def_readwrite("ignore_in_grid", &_Way::ignoreInGrid)
          .def_readwrite("tags", &_Way::keyVals)
          .def_readwrite("direction", &_Way::direction)
          .enum_("constants")
          [
           luabind::value("notSure", 0),
           luabind::value("oneway", 1),
           luabind::value("bidirectional", 2),
           luabind::value("opposite", 3)
          ]
    ];
    // Now call our function in a lua script
	INFO("Parsing speedprofile from " << (argc > 2 ? argv[2] : "profile.lua") );
    if(0 != luaL_dofile(myLuaState, (argc > 2 ? argv[2] : "profile.lua") )) {
        ERR(lua_tostring(myLuaState,-1)<< " occured in scripting block");
    }

    //open utility libraries string library;
    luaL_openlibs(myLuaState);

    /*** End of Scripting Environment Setup; ***/

    unsigned amountOfRAM = 1;
    unsigned installedRAM = GetPhysicalmemory(); 
    if(installedRAM < 2048264) {
        WARN("Machine has less than 2GB RAM.");
    }
/*    if(testDataFile("extractor.ini")) {
        ExtractorConfiguration extractorConfig("extractor.ini");
        unsigned memoryAmountFromFile = atoi(extractorConfig.GetParameter("Memory").c_str());
        if( memoryAmountFromFile != 0 && memoryAmountFromFile <= installedRAM/(1024*1024))
            amountOfRAM = memoryAmountFromFile;
        INFO("Using " << amountOfRAM << " GB of RAM for buffers");
    }
	*/
	
    StringMap stringMap;
    ExtractionContainers externalMemory;

    stringMap[""] = 0;
    extractCallBacks = new ExtractorCallbacks(&externalMemory, &stringMap);
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

    externalMemory.PrepareData(outputFileName, restrictionsFileName, amountOfRAM);

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
    return true;
}
bool wayFunction(_Way w) {
    extractCallBacks->wayFunction(w);
    return true;
}
