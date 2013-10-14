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

#ifndef BASEPARSER_H_
#define BASEPARSER_H_

#include "ExtractorCallbacks.h"
#include "ScriptingEnvironment.h"
#include "../Util/OSRMException.h"
#include "../Util/SimpleLogger.h"

extern "C" {
    #include <lua.h>
    #include <lauxlib.h>
    #include <lualib.h>
}

#include <boost/noncopyable.hpp>

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
    virtual bool ShouldIgnoreRestriction(const std::string& except_tag_string) const;

    ExtractorCallbacks* extractor_callbacks;
    ScriptingEnvironment& scriptingEnvironment;
    lua_State* luaState;
    std::vector<std::string> restriction_exceptions;
    bool use_turn_restrictions;

};

#endif /* BASEPARSER_H_ */
