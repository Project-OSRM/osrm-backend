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

#ifndef LUA_UTIL_
#define LUA_UTIL_

#include <string>
#include <boost/filesystem/convenience.hpp>

bool lua_function_exists(lua_State* lua_state, const char* name)
{
    using namespace luabind;
    object g = globals(lua_state);
    object func = g[name];
    return func && type(func) == LUA_TFUNCTION;
}

void luaAddScriptFolderToLoadPath(lua_State* myLuaState, const char* fileName) {
    //add the folder contain the script to the lua load path, so script can easily require() other lua scripts inside that folder
    //see http://lua-users.org/wiki/PackagePath for details on the package.path syntax
    const boost::filesystem::path profilePath( fileName );
    const std::string folder = profilePath.parent_path().c_str();
    const std::string luaCode = "package.path = \"" + folder + "/?.lua;\" .. package.path";
    luaL_dostring(myLuaState,luaCode.c_str());
}

#endif /* LUA_UTIL_ */
