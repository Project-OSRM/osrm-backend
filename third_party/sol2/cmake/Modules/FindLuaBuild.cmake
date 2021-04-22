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

# # Standard CMake Libraries
include(FindPackageHandleStandardArgs)

# Contain literally everything inside of this function to prevent spillage
function(find_lua_build LUA_VERSION)
	# # # Variables
	# # Core Paths
	string(TOLOWER ${LUA_VERSION} LUA_BUILD_NORMALIZED_LUA_VERSION)
	if (LUA_BUILD_NORMALIZED_LUA_VERSION MATCHES "luajit")
		set(LUA_BUILD_LIBNAME ${LUA_VERSION})
	elseif (BUILD_LUAJIT)
		set(LUA_BUILD_LIBNAME luajit-${LUA_VERSION})
	elseif (LUA_BUILD_NORMALIZED_LUA_VERSION MATCHES "lua")
		set(LUA_BUILD_LIBNAME ${LUA_VERSION})
	elseif (BUILD_LUA)
		set(LUA_BUILD_LIBNAME lua-${LUA_VERSION})
	else()
		set(LUA_BUILD_LIBNAME lua-${LUA_VERSION})
	endif()
	set(LUA_BUILD_TOPLEVEL "${CMAKE_BINARY_DIR}/vendor/${LUA_BUILD_LIBNAME}")
	set(LUA_BUILD_INSTALL_DIR "${LUA_BUILD_TOPLEVEL}")
	# # Misc needed variables
	set(LUA_BUILD_LIBRARY_DESCRIPTION "The base name of the library to build either the static or the dynamic library")

	# Object file suffixes
	if (MSVC)
		set(LUA_BUILD_BUILD_DLL_DEFAULT ON)
		set(LUA_BUILD_OBJECT_FILE_SUFFIX .obj)
	else()
		set(LUA_BUILD_BUILD_DLL_DEFAULT OFF)
		set(LUA_BUILD_OBJECT_FILE_SUFFIX .o)
	endif()

	# # # Options
	option(BUILD_LUA_AS_DLL ${LUA_BUILD_BUILD_DLL_DEFAULT} "Build Lua or LuaJIT as a Shared/Dynamic Link Library")

	STRING(TOLOWER ${LUA_BUILD_LIBNAME} LUA_BUILD_NORMALIZED_LIBNAME)
	if (NOT LUA_LIBRARY_NAME)
		if (LUA_BUILD_NORMALIZED_LIBNAME MATCHES "luajit")
			set(LUA_LIBRARY luajit)
		else()
			set(LUA_LIBRARY ${LUA_BUILD_LIBNAME})
		endif()
	else()
		set(LUA_LIBRARY_NAME ${LUA_LIBRARY_NAME}
			CACHE STRING
			${LUA_BUILD_LIBRARY_DESCRIPTION})
	endif()
	# # Dependent Variables
	# If we're building a DLL, then set the library type to SHARED
	if (BUILD_LUA_AS_DLL)
		set(LUA_BUILD_LIBRARY_TYPE SHARED)
	else()
		set(LUA_BUILD_LIBRARY_TYPE STATIC)
	endif()


	# # # Build Lua
	# # Select either LuaJIT or Vanilla Lua here, based on what we discover
	if (BUILD_LUAJIT OR LUA_BUILD_NORMALIZED_LUA_VERSION MATCHES "luajit")
		include(${CMAKE_CURRENT_LIST_DIR}/FindLuaBuild/LuaJIT.cmake)
		set(LUA_VERSION_STRING ${LUA_JIT_VERSION})
	else()
		include(${CMAKE_CURRENT_LIST_DIR}/FindLuaBuild/LuaVanilla.cmake)
		set(LUA_VERSION_STRING ${LUA_VANILLA_VERSION})
	endif()

	# # Export variables to the parent scope
	set(LUA_LIBRARIES ${LUA_LIBRARIES} PARENT_SCOPE)
	set(LUA_INTERPRETER ${LUA_INTERPRETER} PARENT_SCOPE)
	set(LUA_INCLUDE_DIRS ${LUA_INCLUDE_DIRS} PARENT_SCOPE)
	set(LUA_VERSION_STRING ${LUA_VERSION_STRING} PARENT_SCOPE)
	set(LUABUILD_FOUND TRUE PARENT_SCOPE)
endfunction(find_lua_build)

# Call and then immediately undefine to avoid polluting the global scope
if (LuaBuild_FIND_COMPONENTS)
	list(GET LuaBuild_FIND_COMPONENTS 0 LUA_VERSION)
endif()
if (LuaBuild_FIND_VERSION)
	if (LUA_VERSION)
		set(LUA_VERSION "${LUA_VERSION}-${LuaBuild_FIND_VERSION}")
	else()
		set(LUA_VERSION "${LuaBuild_VERSION}")
	endif()
endif()
if (NOT LUA_VERSION)
	set(LUA_VERSION 5.3.4)
endif()
find_lua_build(${LUA_VERSION})
unset(find_lua_build)

# handle the QUIETLY and REQUIRED arguments and set LUABUILD_FOUND to TRUE if
# all listed variables are TRUE
FIND_PACKAGE_HANDLE_STANDARD_ARGS(LuaBuild
	FOUND_VAR LUABUILD_FOUND
	REQUIRED_VARS LUA_LIBRARIES LUA_INTERPRETER LUA_INCLUDE_DIRS
	VERSION_VAR LUA_VERSION_STRING)
