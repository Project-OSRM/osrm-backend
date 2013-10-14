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

#include "BaseParser.h"

BaseParser::BaseParser(ExtractorCallbacks* ec, ScriptingEnvironment& se) :
extractor_callbacks(ec), scriptingEnvironment(se), luaState(NULL), use_turn_restrictions(true) {
    luaState = se.getLuaStateForThreadID(0);
    ReadUseRestrictionsSetting();
    ReadRestrictionExceptions();
}

void BaseParser::ReadUseRestrictionsSetting() {
    if( 0 != luaL_dostring( luaState, "return use_turn_restrictions\n") ) {
        throw OSRMException(
            /*lua_tostring( luaState, -1 ) + */"ERROR occured in scripting block"
        );
    }
    if( lua_isboolean( luaState, -1) ) {
        use_turn_restrictions = lua_toboolean(luaState, -1);
    }
    if( use_turn_restrictions ) {
        SimpleLogger().Write() << "Using turn restrictions";
    } else {
        SimpleLogger().Write() << "Ignoring turn restrictions";
    }
}

void BaseParser::ReadRestrictionExceptions() {
    if(lua_function_exists(luaState, "get_exceptions" )) {
        //get list of turn restriction exceptions
        luabind::call_function<void>(
            luaState,
            "get_exceptions",
            boost::ref(restriction_exceptions)
        );
        SimpleLogger().Write() << "Found " << restriction_exceptions.size() << " exceptions to turn restriction";
        BOOST_FOREACH(const std::string & str, restriction_exceptions) {
            SimpleLogger().Write() << "   " << str;
        }
    } else {
        SimpleLogger().Write() << "Found no exceptions to turn restrictions";
    }
}

void BaseParser::report_errors(lua_State *L, const int status) const {
    if( 0!=status ) {
        std::cerr << "-- " << lua_tostring(L, -1) << std::endl;
        lua_pop(L, 1); // remove error message
    }
}

void BaseParser::ParseNodeInLua(ImportNode& n, lua_State* localLuaState) {
    luabind::call_function<void>( localLuaState, "node_function", boost::ref(n) );
}

void BaseParser::ParseWayInLua(ExtractionWay& w, lua_State* localLuaState) {
    luabind::call_function<void>( localLuaState, "way_function", boost::ref(w) );
}

bool BaseParser::ShouldIgnoreRestriction(const std::string & except_tag_string) const {
    //should this restriction be ignored? yes if there's an overlap between:
    //a) the list of modes in the except tag of the restriction (except_tag_string), ex: except=bus;bicycle
    //b) the lua profile defines a hierachy of modes, ex: [access, vehicle, bicycle]

    if( "" == except_tag_string ) {
        return false;
    }

    //Be warned, this is quadratic work here, but we assume that
    //only a few exceptions are actually defined.
    std::vector<std::string> exceptions;
    boost::algorithm::split_regex(exceptions, except_tag_string, boost::regex("[;][ ]*"));
    BOOST_FOREACH(std::string& str, exceptions) {
        if( restriction_exceptions.end() != std::find(restriction_exceptions.begin(), restriction_exceptions.end(), str) ) {
            return true;
        }
    }
    return false;
}
