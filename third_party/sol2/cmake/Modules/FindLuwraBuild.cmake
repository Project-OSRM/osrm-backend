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
set(luwra_version 0.5.0)
set(luwra_lib luwra_lib_${luwra_version})

# # Useful locations
set(luwra_build_toplevel "${CMAKE_BINARY_DIR}/vendor/luwra_${luwra_version}")
set(luwra_include_dirs "${luwra_build_toplevel}/lib")

# # luwra library sources
set(luwra_sources luwra.hpp)
prepend(luwra_sources "${luwra_build_toplevel}/lib/" ${luwra_sources})

# # External project to get sources
ExternalProject_Add(LUWRA_BUILD_SOURCE
	BUILD_IN_SOURCE TRUE
	BUILD_ALWAYS FALSE
	# # Use Git to get what we need
	GIT_SHALLOW TRUE
	#GIT_TAG e513907fc8c2d59ebd91cd5992eddf54f7e23e21
	GIT_REPOSITORY https://github.com/vapourismo/luwra.git
	PREFIX ${luwra_build_toplevel}
	SOURCE_DIR ${luwra_build_toplevel}
	DOWNLOAD_DIR ${luwra_build_toplevel}
	TMP_DIR "${luwra_build_toplevel}-tmp"
	STAMP_DIR "${luwra_build_toplevel}-stamp"
	INSTALL_DIR "${luwra_build_toplevel}/local"
	CONFIGURE_COMMAND ""
	BUILD_COMMAND ""
	INSTALL_COMMAND ""
	TEST_COMMAND ""
	BUILD_BYPRODUCTS "${luwra_sources}")

add_library(${luwra_lib} INTERFACE)
add_dependencies(${luwra_lib} LUWRA_BUILD_SOURCE)
target_include_directories(${luwra_lib} INTERFACE ${luwra_include_dirs})
target_link_libraries(${luwra_lib} INTERFACE ${LUA_LIBRARIES})
if (NOT MSVC)
	target_compile_options(${luwra_lib} INTERFACE
		-Wno-noexcept-type -Wno-ignored-qualifiers -Wno-unused-parameter)
endif()

set(LUWRABUILD_FOUND TRUE)
set(LUWRA_LIBRARIES ${luwra_lib})
set(LUWRA_INCLUDE_DIRS ${luwra_include_dirs})

FIND_PACKAGE_HANDLE_STANDARD_ARGS(LuwraBuild
	FOUND_VAR LUWRABUILD_FOUND
	REQUIRED_VARS LUWRA_LIBRARIES LUWRA_INCLUDE_DIRS
	VERSION_VAR luwra_version)
