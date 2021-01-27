# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

# Provides a common-mechanic for extracting Lua versions from Lua/LuaJIT headers
macro(_lua_check_header_version _header_file _prefix)
    # At least 5.[012] have different ways to express the version
    # so all of them need to be tested. Lua 5.2 defines LUA_VERSION
    # and LUA_RELEASE as joined by the C preprocessor, so avoid those.
    file(STRINGS "${_header_file}" lua_version_strings
         REGEX "^#define[ \t]+LUA_(RELEASE[ \t]+\"Lua [0-9]|VERSION([ \t]+\"Lua [0-9]|_[MR])).*")

    string(REGEX REPLACE ".*;#define[ \t]+LUA_VERSION_MAJOR[ \t]+\"([0-9])\"[ \t]*;.*" "\\1" ${_prefix}_VERSION_MAJOR ";${lua_version_strings};")
    if (${_prefix}_VERSION_MAJOR MATCHES "^[0-9]+$")
        string(REGEX REPLACE ".*;#define[ \t]+LUA_VERSION_MINOR[ \t]+\"([0-9])\"[ \t]*;.*" "\\1" "${_prefix}_VERSION_MINOR" ";${lua_version_strings};")
        string(REGEX REPLACE ".*;#define[ \t]+LUA_VERSION_RELEASE[ \t]+\"([0-9])\"[ \t]*;.*" "\\1" "${_prefix}_VERSION_PATCH" ";${lua_version_strings};")
        set(${_prefix}_VERSION_STRING "${${_prefix}_VERSION_MAJOR}.${${_prefix}_VERSION_MINOR}.${${_prefix}_VERSION_PATCH}")
    else ()
        string(REGEX REPLACE ".*;#define[ \t]+LUA_RELEASE[ \t]+\"Lua ([0-9.]+)\"[ \t]*;.*" "\\1" ${_prefix}_VERSION_STRING ";${lua_version_strings};")
        if (NOT LUA_VERSION_STRING MATCHES "^[0-9.]+$")
            string(REGEX REPLACE ".*;#define[ \t]+LUA_VERSION[ \t]+\"Lua ([0-9.]+)\"[ \t]*;.*" "\\1" ${_prefix}_VERSION_STRING ";${lua_version_strings};")
        endif ()
        string(REGEX REPLACE "^([0-9]+)\\.[0-9.]*$" "\\1" ${_prefix}_VERSION_MAJOR "${${_prefix}_VERSION_STRING}")
        string(REGEX REPLACE "^[0-9]+\\.([0-9]+)[0-9.]*$" "\\1" ${_prefix}_VERSION_MINOR "${${_prefix}_VERSION_STRING}")
        string(REGEX REPLACE "^[0-9]+\\.[0-9]+\\.([0-9]).*" "\\1" ${_prefix}_VERSION_PATCH "${${_prefix}_VERSION_STRING}")
    endif ()

    unset(lua_version_strings)
endmacro(_lua_check_header_version)