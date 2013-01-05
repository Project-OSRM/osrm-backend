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



#ifndef LUAUTIL_H_
#define LUAUTIL_H_

#include <iostream>
#include <string>
#include <boost/filesystem/convenience.hpp>

template<typename T>
void LUA_print(T number) {
  std::cout << "[LUA] " << number << std::endl;
}

// Check if the lua function <name> is defined
inline bool lua_function_exists(lua_State* lua_state, const char* name) {
    luabind::object g = luabind::globals(lua_state);
    luabind::object func = g[name];
    return func && (luabind::type(func) == LUA_TFUNCTION);
}

// Add the folder contain the script to the lua load path, so script can easily require() other lua scripts inside that folder, or subfolders.
// See http://lua-users.org/wiki/PackagePath for details on the package.path syntax.
inline void luaAddScriptFolderToLoadPath(lua_State* myLuaState, const char* fileName) {
    const boost::filesystem::path profilePath( fileName );
    std::string folder = profilePath.parent_path().string();
    //TODO: This code is most probably not Windows safe since it uses UNIX'ish path delimiters
    const std::string luaCode = "package.path = \"" + folder + "/?.lua;profiles/?.lua;\" .. package.path";
    luaL_dostring( myLuaState, luaCode.c_str() );
}

#endif /* LUAUTIL_H_ */
