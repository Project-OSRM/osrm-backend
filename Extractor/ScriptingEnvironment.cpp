/*

Copyright (c) 2013, Project OSRM, Dennis Luxen, others
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list
of conditions and the following disclaimer.
Redistributions in binary form must reproduce the above copyright notice, this
list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include "ScriptingEnvironment.h"

#include "ExtractionHelperFunctions.h"
#include "ExtractionWay.h"
#include "../DataStructures/ImportNode.h"
#include "../Util/LuaUtil.h"
#include "../Util/OpenMPWrapper.h"
#include "../Util/OSRMException.h"
#include "../Util/SimpleLogger.h"
#include "../typedefs.h"

ScriptingEnvironment::ScriptingEnvironment() {}
ScriptingEnvironment::ScriptingEnvironment(const char *file_name)
{
    SimpleLogger().Write() << "Using script " << file_name;

    // Create a new lua state
    for (int i = 0; i < omp_get_max_threads(); ++i)
    {
        lua_state_vector.push_back(luaL_newstate());
    }

// Connect LuaBind to this lua state for all threads
#pragma omp parallel
    {
        lua_State *lua_state = getLuaStateForThreadID(omp_get_thread_num());
        luabind::open(lua_state);
        // open utility libraries string library;
        luaL_openlibs(lua_state);

        luaAddScriptFolderToLoadPath(lua_state, file_name);

        // Add our function to the state's global scope
        luabind::module(lua_state)[
            luabind::def("print", LUA_print<std::string>),
            luabind::def("durationIsValid", durationIsValid),
            luabind::def("parseDuration", parseDuration)
        ];

        luabind::module(lua_state)[luabind::class_<HashTable<std::string, std::string>>("keyVals")
                                        .def("Add", &HashTable<std::string, std::string>::Add)
                                        .def("Find", &HashTable<std::string, std::string>::Find)
                                        .def("Holds", &HashTable<std::string, std::string>::Holds)];

        luabind::module(lua_state)[luabind::class_<ImportNode>("Node")
                                        .def(luabind::constructor<>())
                                        .def_readwrite("lat", &ImportNode::lat)
                                        .def_readwrite("lon", &ImportNode::lon)
                                        .def_readonly("id", &ImportNode::id)
                                        .def_readwrite("bollard", &ImportNode::bollard)
                                        .def_readwrite("traffic_light", &ImportNode::trafficLight)
                                        .def_readwrite("tags", &ImportNode::keyVals)];

        luabind::module(lua_state)
            [luabind::class_<ExtractionWay>("Way")
                 .def(luabind::constructor<>())
                 .def_readonly("id", &ExtractionWay::id)
                 .def_readwrite("name", &ExtractionWay::name)
                 .def_readwrite("speed", &ExtractionWay::speed)
                 .def_readwrite("backward_speed", &ExtractionWay::backward_speed)
                 .def_readwrite("duration", &ExtractionWay::duration)
                 .def_readwrite("type", &ExtractionWay::type)
                 .def_readwrite("access", &ExtractionWay::access)
                 .def_readwrite("roundabout", &ExtractionWay::roundabout)
                 .def_readwrite("is_access_restricted", &ExtractionWay::isAccessRestricted)
                 .def_readwrite("ignore_in_grid", &ExtractionWay::ignoreInGrid)
                 .def_readwrite("tags", &ExtractionWay::keyVals)
                 .def_readwrite("direction", &ExtractionWay::direction)
                 .enum_("constants")[
                     luabind::value("notSure", 0),
                     luabind::value("oneway", 1),
                     luabind::value("bidirectional", 2),
                     luabind::value("opposite", 3)
                 ]];

        // fails on c++11/OS X 10.9
        luabind::module(lua_state)[luabind::class_<std::vector<std::string>>("vector").def(
            "Add",
            static_cast<void (std::vector<std::string>::*)(const std::string &)>(
                &std::vector<std::string>::push_back))];

        if (0 != luaL_dofile(lua_state, file_name))
        {
            throw OSRMException("ERROR occured in scripting block");
        }
    }
}

ScriptingEnvironment::~ScriptingEnvironment()
{
    for (unsigned i = 0; i < lua_state_vector.size(); ++i)
    {
        //        lua_state_vector[i];
    }
}

lua_State *ScriptingEnvironment::getLuaStateForThreadID(const int id) { return lua_state_vector[id]; }
