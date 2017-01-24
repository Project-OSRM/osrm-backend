#ifndef LUA_UTIL_HPP
#define LUA_UTIL_HPP

extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

#include <boost/filesystem/convenience.hpp>

#include <iostream>
#include <string>

namespace osrm
{
namespace util
{

// Add the folder contain the script to the lua load path, so script can easily require() other lua
// scripts inside that folder, or subfolders.
// See http://lua-users.org/wiki/PackagePath for details on the package.path syntax.
inline void luaAddScriptFolderToLoadPath(lua_State *lua_state, const char *file_name)
{
    boost::filesystem::path profile_path = boost::filesystem::canonical(file_name);
    std::string folder = profile_path.parent_path().generic_string();
    const std::string lua_code = "package.path = \"" + folder + "/?.lua;\" .. package.path";
    luaL_dostring(lua_state, lua_code.c_str());
}
}
}

#endif // LUA_UTIL_HPP
