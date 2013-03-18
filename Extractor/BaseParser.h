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

#ifndef BASEPARSER_H_
#define BASEPARSER_H_

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

#include <boost/noncopyable.hpp>

#include "ExtractorCallbacks.h"
#include "ScriptingEnvironment.h"

class BaseParser : boost::noncopyable {
public:
    BaseParser(ExtractorCallbacks* ec, ScriptingEnvironment& se);
    virtual ~BaseParser() {}
    virtual bool ReadHeader() = 0;
    virtual bool Parse() = 0;

    virtual void ParseNodeInLua(ImportNode& n, lua_State* luaStateForThread);
    virtual void ParseWayInLua(ExtractionWay& n, lua_State* luaStateForThread);
    virtual void report_errors(lua_State *L, const int status) const;

protected:   
    virtual void ReadUseRestrictionsSetting();
    virtual void ReadRestrictionExceptions();
    virtual void ReadModes();
    virtual bool ShouldIgnoreRestriction(const std::string& except_tag_string) const;
    
    ExtractorCallbacks* extractor_callbacks;
    ScriptingEnvironment& scriptingEnvironment;
    lua_State* luaState;
    std::vector<std::string> restriction_exceptions;
    bool use_turn_restrictions;
    std::vector<std::string> modes;
};

#endif /* BASEPARSER_H_ */
