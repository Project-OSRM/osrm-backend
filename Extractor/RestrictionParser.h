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

#ifndef RESTRICTION_PARSER_H_
#define RESTRICTION_PARSER_H_

#include "../DataStructures/Restriction.h"

#include <osmium/osm.hpp>
#include <osmium/tags/regex_filter.hpp>

#include <variant/optional.hpp>

#include <string>
#include <vector>

struct lua_State;
class ScriptingEnvironment;

class RestrictionParser
{
  public:
    RestrictionParser(ScriptingEnvironment &scripting_environment);

    mapbox::util::optional<InputRestrictionContainer> TryParse(osmium::Relation& relation) const;

    void ReadUseRestrictionsSetting();
    void ReadRestrictionExceptions();
  private:
    bool ShouldIgnoreRestriction(const std::string &except_tag_string) const;

    lua_State *lua_state;
    std::vector<std::string> restriction_exceptions;
    bool use_turn_restrictions;
};

#endif /* RESTRICTION_PARSER_H_ */
