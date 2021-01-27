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

include(ExternalProject)
include(FindPackageHandleStandardArgs)
include(Common/Core)

# # Base variables
set(luabridge_version 1.0.2)
set(luabridge_lib luabridge_lib_${luabridge_version})

# # Useful locations
set(luabridge_build_toplevel "${CMAKE_BINARY_DIR}/vendor/luabridge_${luabridge_version}")
set(luabridge_include_dirs "${luabridge_build_toplevel}/Source")

# # luabridge library sources
set(luabridge_sources LuaBridge/LuaBridge.h)
prepend(luabridge_sources "${luabridge_build_toplevel}/Source/" ${luabridge_sources})

# # External project to get sources
ExternalProject_Add(LUABRIDGE_BUILD_SOURCE
	BUILD_IN_SOURCE TRUE
	BUILD_ALWAYS FALSE
	# # Use Git to get what we need
	GIT_SHALLOW TRUE
	GIT_SUBMODULES ""
	GIT_REPOSITORY https://github.com/ThePhD/LuaBridge.git
	PREFIX ${luabridge_build_toplevel}
	SOURCE_DIR ${luabridge_build_toplevel}
	DOWNLOAD_DIR ${luabridge_build_toplevel}
	TMP_DIR "${luabridge_build_toplevel}-tmp"
	STAMP_DIR "${luabridge_build_toplevel}-stamp"
	INSTALL_DIR "${luabridge_build_toplevel}/local"
	CONFIGURE_COMMAND ""
	BUILD_COMMAND ""
	INSTALL_COMMAND ""
	TEST_COMMAND ""
	BUILD_BYPRODUCTS "${luabridge_sources}")

add_library(${luabridge_lib} INTERFACE)
add_dependencies(${luabridge_lib} LUABRIDGE_BUILD_SOURCE)
target_include_directories(${luabridge_lib} INTERFACE ${luabridge_include_dirs})
target_link_libraries(${luabridge_lib} INTERFACE ${LUA_LIBRARIES})
if (NOT MSVC)
	target_compile_options(${luabridge_lib} INTERFACE
		-Wno-noexcept-type -Wno-ignored-qualifiers -Wno-unused-parameter)
endif()
set(LUABRIDGEBUILD_FOUND TRUE)
set(LUABRIDGE_LIBRARIES ${luabridge_lib})
set(LUABRIDGE_INCLUDE_DIRS ${luabridge_include_dirs})

FIND_PACKAGE_HANDLE_STANDARD_ARGS(LuabridgeBuild
	FOUND_VAR LUABRIDGEBUILD_FOUND
	REQUIRED_VARS LUABRIDGE_LIBRARIES LUABRIDGE_INCLUDE_DIRS
	VERSION_VAR luabridge_version)


