
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
set(kaguya_version 1.3.2)
set(kaguya_lib kaguya_lib_${kaguya_version})

# # Useful locations
set(kaguya_build_toplevel "${CMAKE_BINARY_DIR}/vendor/kaguya_${kaguya_version}")
set(kaguya_include_dirs "${kaguya_build_toplevel}/include")

# # kaguya library sources
set(kaguya_sources kaguya/kaguya.hpp)
prepend(kaguya_sources "${kaguya_build_toplevel}/include/" ${kaguya_sources})

# # External project to get sources
ExternalProject_Add(KAGUYA_BUILD_SOURCE
	BUILD_IN_SOURCE TRUE
	BUILD_ALWAYS FALSE
	# # Use Git to get what we need
	GIT_SHALLOW TRUE
	GIT_SUBMODULES ""
	GIT_REPOSITORY https://github.com/satoren/kaguya.git
	PREFIX ${kaguya_build_toplevel}
	SOURCE_DIR ${kaguya_build_toplevel}
	DOWNLOAD_DIR ${kaguya_build_toplevel}
	TMP_DIR "${kaguya_build_toplevel}-tmp"
	STAMP_DIR "${kaguya_build_toplevel}-stamp"
	INSTALL_DIR "${kaguya_build_toplevel}/local"
	CONFIGURE_COMMAND ""
	BUILD_COMMAND ""
	INSTALL_COMMAND ""
	TEST_COMMAND ""
	BUILD_BYPRODUCTS "${kaguya_sources}")

add_library(${kaguya_lib} INTERFACE)
add_dependencies(${kaguya_lib} KAGUYA_BUILD_SOURCE)
target_include_directories(${kaguya_lib} INTERFACE ${kaguya_include_dirs})
target_link_libraries(${kaguya_lib} INTERFACE ${LUA_LIBRARIES})
if (NOT MSVC)
	target_compile_options(${kaguya_lib} INTERFACE
		-Wno-noexcept-type -Wno-ignored-qualifiers -Wno-unused-parameter)
endif()

set(KAGUYABUILD_FOUND TRUE)
set(KAGUYA_LIBRARIES ${kaguya_lib})
set(KAGUYA_INCLUDE_DIRS ${kaguya_include_dirs})

FIND_PACKAGE_HANDLE_STANDARD_ARGS(KaguyaBuild
	FOUND_VAR KAGUYABUILD_FOUND
	REQUIRED_VARS KAGUYA_LIBRARIES KAGUYA_INCLUDE_DIRS
	VERSION_VAR kaguya_version)