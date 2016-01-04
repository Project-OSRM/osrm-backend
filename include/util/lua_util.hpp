#ifndef LUA_UTIL_HPP
#define LUA_UTIL_HPP

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

#include <boost/filesystem/convenience.hpp>
#include <luabind/luabind.hpp>

#include <iostream>
#include <string>

template <typename T> void LUA_print(T output) { std::cout << "[LUA] " << output << std::endl; }

// Check if the lua function <name> is defined
inline bool lua_function_exists(lua_State *lua_state, const char *name)
{
    luabind::object globals_table = luabind::globals(lua_state);
    luabind::object lua_function = globals_table[name];
    return lua_function && (luabind::type(lua_function) == LUA_TFUNCTION);
}

// Add the folder contain the script to the lua load path, so script can easily require() other lua
// scripts inside that folder, or subfolders.
// See http://lua-users.org/wiki/PackagePath for details on the package.path syntax.
inline void luaAddScriptFolderToLoadPath(lua_State *lua_state, const char *file_name)
{
    boost::filesystem::path profile_path = boost::filesystem::canonical(file_name);
    std::string folder = profile_path.parent_path().string();
    // TODO: This code is most probably not Windows safe since it uses UNIX'ish path delimiters
    const std::string lua_code = "package.path = \"" + folder + "/?.lua;\" .. package.path";
    luaL_dostring(lua_state, lua_code.c_str());
}

#endif // LUA_UTIL_HPP
