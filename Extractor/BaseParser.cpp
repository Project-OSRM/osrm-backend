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
