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
if (Catch_FIND_VERSION)
	set(catch_version ${Catch_FIND_VERSION})
else()
	set(catch_version 2.1.2)
endif()

set(catch_lib catch_lib_${catch_version})

# # Useful locations
set(catch_build_toplevel "${CMAKE_BINARY_DIR}/vendor/catch_${catch_version}")
set(catch_include_dirs "${catch_build_toplevel}")

# # catch library sources
set(catch_sources catch.hpp)
prepend(catch_sources "${catch_build_toplevel}/" ${catch_sources})

# # !! CMake 3.5 does not have DOWNLOAD_NO_EXTRACT e.e
# # Now I know why people don't like CMake that much: the earlier versions were kind of garbage
# # External project to get sources
#ExternalProject_Add(CATCH_BUILD_SOURCE
#	BUILD_IN_SOURCE TRUE
#	BUILD_ALWAYS FALSE
#	DOWNLOAD_NO_EXTRACT TRUE
#	URL https://github.com/catchorg/Catch2/releases/download/v${catch_version}/catch.hpp
#	TLS_VERIFY TRUE
#	PREFIX ${catch_build_toplevel}
#	SOURCE_DIR ${catch_build_toplevel}
#	DOWNLOAD_DIR ${catch_build_toplevel}
#	TMP_DIR "${catch_build_toplevel}-tmp"
#	STAMP_DIR "${catch_build_toplevel}-stamp"
#	INSTALL_DIR "${catch_build_toplevel}/local"
#	CONFIGURE_COMMAND ""
#	BUILD_COMMAND ""
#	INSTALL_COMMAND ""
#	TEST_COMMAND ""
#	BUILD_BYPRODUCTS "${catch_sources}")

file(MAKE_DIRECTORY "${catch_build_toplevel}")
file(DOWNLOAD https://github.com/catchorg/Catch2/releases/download/v${catch_version}/catch.hpp ${catch_sources})

add_library(${catch_lib} INTERFACE)
# add_dependencies(${catch_lib} CATCH_BUILD_SOURCE)
target_include_directories(${catch_lib} INTERFACE ${catch_include_dirs})

if (MSVC)
	target_compile_options(${catch_lib} INTERFACE
		/D_SILENCE_CXX17_UNCAUGHT_EXCEPTION_DEPRECATION_WARNING)
endif()

set(CATCH_FOUND TRUE)
set(CATCH_LIBRARIES ${catch_lib})
set(CATCH_INCLUDE_DIRS ${catch_include_dirs})

FIND_PACKAGE_HANDLE_STANDARD_ARGS(Catch
	FOUND_VAR CATCH_FOUND
	REQUIRED_VARS CATCH_LIBRARIES CATCH_INCLUDE_DIRS
	VERSION_VAR catch_version)
