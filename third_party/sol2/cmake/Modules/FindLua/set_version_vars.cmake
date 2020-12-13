# # # # sol2
# The MIT License (MIT)
# 
# Copyright (c) 2013-2018 Rapptz, ThePhD, and contributors
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy of
# this software and associated documentation files (the "Software"), to deal in
# the Software without restriction, including without limitation the rights to
# use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
# the Software, and to permit persons to whom the Software is furnished to do so,
# subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
# FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
# COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
# IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
# CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

# Uses the Lua version and calculates the names of the include directories and library
# name accordingly.
#
# ::
#   _prefix     - Prefix added to all variables exposed, e.g. "lua" or "luajit"
#   _lua_suffix - Name suffix, typically only used for specifiying luajit, which is "jit"

function(_lua_set_version_vars _prefix _lua_suffix)
    set(LUA_VERSIONS5 5.3 5.2 5.1 5.0)

    if (${_prefix}_FIND_VERSION_EXACT)
        if (${_prefix}_FIND_VERSION_COUNT GREATER 1)
            set(_${_prefix}_append_versions ${${_prefix}_FIND_VERSION_MAJOR}.${${_prefix}_FIND_VERSION_MINOR})
        endif ()
    elseif (${_prefix}_FIND_VERSION)
        # once there is a different major version supported this should become a loop
        if (NOT ${_prefix}_FIND_VERSION_MAJOR GREATER 5)
            if (${_prefix}_FIND_VERSION_COUNT EQUAL 1)
                set(_${_prefix}_append_versions ${${_prefix}_VERSIONS5})
            else ()
                foreach (subver IN LISTS ${_prefix}_VERSIONS5)
                    if (NOT subver VERSION_LESS ${${_prefix}_FIND_VERSION})
                        list(APPEND _${_prefix}_append_versions ${subver})
                    endif ()
                endforeach ()
            endif ()
        endif ()
    else ()
        # once there is a different major version supported this should become a loop
        set(_${_prefix}_append_versions ${LUA_VERSIONS5})
    endif ()

    list(APPEND _${_prefix}_include_subdirs "include/lua" "include")

    foreach (ver IN LISTS _${_prefix}_append_versions)
        string(REGEX MATCH "^([0-9]+)\\.([0-9]+)$" _ver "${ver}")
        list(APPEND _${_prefix}_include_subdirs
             include/lua${_lua_suffix}${CMAKE_MATCH_1}${CMAKE_MATCH_2}
             include/lua${_lua_suffix}${CMAKE_MATCH_1}.${CMAKE_MATCH_2}
             include/lua${_lua_suffix}-${CMAKE_MATCH_1}.${CMAKE_MATCH_2}
        )

        # LuaJIT hides itself as Lua lib (maintaining ABI compat)
        list(APPEND _${_prefix}_library_names
             lua${CMAKE_MATCH_1}${CMAKE_MATCH_2}
             lua${CMAKE_MATCH_1}.${CMAKE_MATCH_2}
             lua.${CMAKE_MATCH_1}.${CMAKE_MATCH_2}
             lua-${CMAKE_MATCH_1}.${CMAKE_MATCH_2}
        )
    endforeach ()

    set(_${_prefix}_include_subdirs "${_${_prefix}_include_subdirs}" PARENT_SCOPE)
    set(_${_prefix}_append_versions "${_${_prefix}_append_versions}" PARENT_SCOPE)
    set(_${_prefix}_library_names "${_${_prefix}_library_names}" PARENT_SCOPE)
endfunction(_lua_set_version_vars)