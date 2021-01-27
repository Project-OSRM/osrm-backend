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
include(ExternalProject)
include(FindPackageHandleStandardArgs)
include(Common/Core)

# # Base variables
set(toluapp_version 1.0.93)

# # Useful locations
set(toluapp_build_toplevel "${CMAKE_BINARY_DIR}/vendor/toluapp_${toluapp_version}")
set(toluapp_include_dirs "${toluapp_build_toplevel}/include")

# # ToLua library sources
set(toluapp_sources tolua_event.c tolua_event.h tolua_is.c tolua_map.c tolua_push.c tolua_to.c tolua_compat.h tolua_compat.c)
prepend(toluapp_sources "${toluapp_build_toplevel}/src/lib/" ${toluapp_sources})
list(APPEND toluapp_sources "${toluapp_build_toplevel}/include/tolua++.h")

# # External project to get sources
ExternalProject_Add(TOLUAPP_BUILD_SOURCE
	BUILD_IN_SOURCE TRUE
	BUILD_ALWAYS FALSE
	# # Use Git to get what we need
	#GIT_SUBMODULES ""
	GIT_SHALLOW TRUE
	GIT_REPOSITORY https://github.com/ThePhD/toluapp
	PREFIX ${toluapp_build_toplevel}
	SOURCE_DIR ${toluapp_build_toplevel}
	DOWNLOAD_DIR ${toluapp_build_toplevel}
	TMP_DIR "${toluapp_build_toplevel}-tmp"
	STAMP_DIR "${toluapp_build_toplevel}-stamp"
	INSTALL_DIR "${toluapp_build_toplevel}/local"
	CONFIGURE_COMMAND ""
	BUILD_COMMAND ""
	INSTALL_COMMAND ""
	TEST_COMMAND ""
	BUILD_BYPRODUCTS "${toluapp_sources}")

set(toluapp_lib toluapp_lib_5.2.4)
add_library(${toluapp_lib} SHARED ${toluapp_sources})
add_dependencies(${toluapp_lib} TOLUAPP_BUILD_SOURCE)
set_target_properties(${toluapp_lib} PROPERTIES
	OUTPUT_NAME toluapp-${toluapp_version}
	POSITION_INDEPENDENT_CODE TRUE)
target_include_directories(${toluapp_lib}
	PUBLIC ${toluapp_include_dirs})
target_link_libraries(${toluapp_lib} PRIVATE ${LUA_LIBRARIES})
if (MSVC)
	target_compile_options(${toluapp_lib}
		PRIVATE /W1)
	target_compile_definitions(${toluapp_lib}
		PRIVATE TOLUA_API=__declspec(dllexport))
else()
	target_compile_options(${toluapp_lib}
		PRIVATE -w
		INTERFACE -Wno-noexcept-type
		PUBLIC -Wno-ignored-qualifiers -Wno-unused-parameter)
endif()
if (CMAKE_DL_LIBS)
	target_link_libraries(${toluapp_lib} PRIVATE ${CMAKE_DL_LIBS})
endif()
# add compatibility define
target_compile_definitions(${toluapp_lib}
		PRIVATE COMPAT53_PREFIX=toluapp_compat53)

# # Variables required by ToLuaBuild
set(TOLUAPP_LIBRARIES ${toluapp_lib})
set(TOLUAPP_INCLUDE_DIRS ${toluapp_include_dirs})
set(TOLUAPPBUILD_FOUND TRUE)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(ToLuappBuild
	FOUND_VAR TOLUAPPBUILD_FOUND
	REQUIRED_VARS TOLUAPP_LIBRARIES TOLUAPP_INCLUDE_DIRS
	VERSION_VAR toluapp_version)
