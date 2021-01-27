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

# Include guard
if(_lua_vanilla_build_included)
	return()
endif(_lua_vanilla_build_included)
set(_lua_vanilla_build_included true)

# import necessary standard modules
include(ExternalProject)
# import other modules
include(Common/Core)

# Latest versions for specific sub-versions of Lua
set(LUA_VANILLA_5.1_LATEST_VERSION 5.1.5)
set(LUA_VANILLA_5.2_LATEST_VERSION 5.2.4)
set(LUA_VANILLA_5.3_LATEST_VERSION 5.3.4)

# exact version, coming from CI: pull directly from Lua and use external project to build
# list of known md5 / sha1: must update when there are changes
set(LUA_VANILLA_MD5_5.3.4 53a9c68bcc0eda58bdc2095ad5cdfc63)
set(LUA_VANILLA_SHA1_5.3.4 79790cfd40e09ba796b01a571d4d63b52b1cd950)
set(LUA_VANILLA_MD5_5.3.3 703f75caa4fdf4a911c1a72e67a27498)
set(LUA_VANILLA_SHA1_5.3.3 a0341bc3d1415b814cc738b2ec01ae56045d64ef)
set(LUA_VANILLA_MD5_5.3.2 33278c2ab5ee3c1a875be8d55c1ca2a1)
set(LUA_VANILLA_SHA1_5.3.2 7a47adef554fdca7d0c5536148de34579134a973)
set(LUA_VANILLA_MD5_5.3.1 797adacada8d85761c079390ff1d9961)
set(LUA_VANILLA_SHA1_5.3.1 1676c6a041d90b6982db8cef1e5fb26000ab6dee)
set(LUA_VANILLA_MD5_5.3.0 a1b0a7e92d0c85bbff7a8d27bf29f8af)
set(LUA_VANILLA_SHA1_5.3.0 1c46d1c78c44039939e820126b86a6ae12dadfba)
set(LUA_VANILLA_MD5_5.2.4 913fdb32207046b273fdb17aad70be13)
set(LUA_VANILLA_SHA1_5.2.4 ef15259421197e3d85b7d6e4871b8c26fd82c1cf)
set(LUA_VANILLA_MD5_5.2.3 dc7f94ec6ff15c985d2d6ad0f1b35654)
set(LUA_VANILLA_SHA1_5.2.3 926b7907bc8d274e063d42804666b40a3f3c124c)
set(LUA_VANILLA_MD5_5.2.2 efbb645e897eae37cad4344ce8b0a614)
set(LUA_VANILLA_SHA1_5.2.2 0857e41e5579726a4cb96732e80d7aa47165eaf5)
set(LUA_VANILLA_MD5_5.2.1 ae08f641b45d737d12d30291a5e5f6e3)
set(LUA_VANILLA_SHA1_5.2.1 6bb1b0a39b6a5484b71a83323c690154f86b2021)
set(LUA_VANILLA_MD5_5.2.0 f1ea831f397214bae8a265995ab1a93e)
set(LUA_VANILLA_SHA1_5.2.0 08f84c355cdd646f617f09cebea48bd832415829)
set(LUA_VANILLA_MD5_5.1.5 2e115fe26e435e33b0d5c022e4490567)
set(LUA_VANILLA_SHA1_5.1.5 b3882111ad02ecc6b972f8c1241647905cb2e3fc)
set(LUA_VANILLA_MD5_5.1.4 d0870f2de55d59c1c8419f36e8fac150)
set(LUA_VANILLA_SHA1_5.1.4 2b11c8e60306efb7f0734b747588f57995493db7)
set(LUA_VANILLA_MD5_5.1.3 a70a8dfaa150e047866dc01a46272599)
set(LUA_VANILLA_SHA1_5.1.3 89bc9f5a351402565b8077e8123327e7cd15f004)
set(LUA_VANILLA_MD5_5.1.2 687ce4c2a1ddff18f1008490fdc4e5e0)
set(LUA_VANILLA_SHA1_5.1.2 8a460d2d7e70e93cb72bf3d584405464763cb5f0)
set(LUA_VANILLA_MD5_5.1.1 22f4f912f20802c11006fe9b84d5c461)
set(LUA_VANILLA_SHA1_5.1.1 be13878ceef8e1ee7a4201261f0adf09f89f1005)
set(LUA_VANILLA_MD5_5.1 3e8dfe8be00a744cec2f9e766b2f2aee)
set(LUA_VANILLA_SHA1_5.1 1ae9ec317511d525c7999c842ca0b1ddde84e374)
set(LUA_VANILLA_MD5_5.0.3 feee27132056de2949ce499b0ef4c480)
set(LUA_VANILLA_SHA1_5.0.3 e7e91f78b8a8deb09b13436829bed557a46af8ae)
set(LUA_VANILLA_MD5_5.0.2 dea74646b7e5c621fef7174df83c34b1)
set(LUA_VANILLA_SHA1_5.0.2 a200cfd20a9a4c7da1206ae45dddf26186a9e0e7)
set(LUA_VANILLA_MD5_5.0.1 e0a450d84971a3f4563b98172d1e382c)
set(LUA_VANILLA_SHA1_5.0.1 03b47b4785178aca583333f01d8726a8ab9f7ae7)
set(LUA_VANILLA_MD5_5.0 6f14803fad389fb1cb15d17edfeddd91)
set(LUA_VANILLA_SHA1_5.0 88b1bc057857c0db5ace491c4af2c917a2b803bf)
set(LUA_VANILLA_MD5_4.0.1 a31d963dbdf727f9b34eee1e0d29132c)
set(LUA_VANILLA_SHA1_4.0.1 12f1864a7ecd4b8011862a07fa3f177b2e80e7d3)
set(LUA_VANILLA_MD5_4.0 be11522d46d33a931868c03694aaeeef)
set(LUA_VANILLA_SHA1_4.0 8d432c73ef6e98b81d252114be1a83182cc9607a)
set(LUA_VANILLA_MD5_3.2.2 374ba5c4839709922de40b8d10382705)
set(LUA_VANILLA_SHA1_3.2.2 fa50ff14c00d8523c8a3d1d3f4887ecc4400d0c3)
set(LUA_VANILLA_MD5_3.2.1 47264a1978df49fc1dea6ffcddb05b21)
set(LUA_VANILLA_SHA1_3.2.1 d43af5a1c7a65c0ddb4b0ac06c29ecf4cdd22367)
set(LUA_VANILLA_MD5_3.2 a6552da3d40ae9b04489a788262279e8)
set(LUA_VANILLA_SHA1_3.2 84cf9f0e7d00eed3ea8b4ac2b84254b714510b34)
set(LUA_VANILLA_MD5_3.1 d677f3827167eefdefc7b211397cfdfb)
set(LUA_VANILLA_SHA1_3.1 509485e3baafd946f4ffe2a984f8a63746adc32a)
set(LUA_VANILLA_MD5_3.0 997558ae76c2f1cd1e10fd3835c45c6a)
set(LUA_VANILLA_SHA1_3.0 5c8c910353f717ba29b4fe7d538994454229b335)
set(LUA_VANILLA_MD5_2.5 da915d58904e75b9b0fc18147e19b0bb)
set(LUA_VANILLA_SHA1_2.5 7920e12c40242932c22fa261ff114cc485a39d99)
set(LUA_VANILLA_MD5_2.4 5d035cc244285c1dbbcaaa0908b58965)
set(LUA_VANILLA_SHA1_2.4 74036935b36e6ae4ed17bd7a9408154f9a4a6b17)
set(LUA_VANILLA_MD5_2.2 a298b58e197ff8168ec907d6145252ef)
set(LUA_VANILLA_SHA1_2.2 2d8b1df94b2fb76f0f16ca1ddc54d5186b10df4b)
set(LUA_VANILLA_MD5_2.1 053a9f6728cc56f6a23716a6a1ede595)
set(LUA_VANILLA_SHA1_2.1 b9a797547f480bcb58b5d3da846c8ac8d2201df0)
set(LUA_VANILLA_MD5_1.1 9f83141cc8ea362497e272071eda5cf6)
set(LUA_VANILLA_SHA1_1.1 67209701eec5cc633e829d023fbff62d5d6c8e5e)
set(LUA_VANILLA_MD5_1.0 96e8399fc508d128badd8ac3aa8f2119)
set(LUA_VANILLA_SHA1_1.0 6a82d2ae7ce9ad98c7b4824a325b91522c0d6ebb)

# Clean up some variables
if (LUA_VERSION MATCHES "^([0-9]+)\\.([0-9]+)\\.([0-9]+)$")
	# probably okay!
	set(LUA_VANILLA_VERSION ${CMAKE_MATCH_1}.${CMAKE_MATCH_2}.${CMAKE_MATCH_3})
elseif (LUA_VERSION MATCHES "([0-9]+)\\.([0-9]+)")
	# extend version number with prefix
	if (${CMAKE_MATCH_1} EQUAL 5)
		if (${CMAKE_MATCH_2} EQUAL 1)
			set(LUA_VANILLA_VERSION ${LUA_VANILLA_5.1_LATEST_VERSION})
		elseif (${CMAKE_MATCH_2} EQUAL 2)
			set(LUA_VANILLA_VERSION ${LUA_VANILLA_5.2_LATEST_VERSION})
		elseif (${CMAKE_MATCH_2} EQUAL 3)
			set(LUA_VANILLA_VERSION ${LUA_VANILLA_5.3_LATEST_VERSION})
		else()			
			# default to whatever the first two
			# numbers happen to be, plus build 0
			set(LUA_VANILLA_VERSION ${CMAKE_MATCH_1}.${CMAKE_MATCH_2}.0)
		endif()
	endif()
	if(NOT LUA_VANILLA_VERSION)			
		# default to whatever the first two
		# numbers happen to be, plus build 0
		set(LUA_VANILLA_VERSION ${CMAKE_MATCH_1}.${CMAKE_MATCH_2}.0)
	endif()
else()
	message(FATAL_ERROR "Cannot deduce the proper Lua version from ${LUA_VERSION}")
endif()

FIND_PACKAGE_MESSAGE(LUABUILD
	"Selecting PUC-RIO Lua ${LUA_VANILLA_VERSION} from '${LUA_VERSION}' and building a ${LUA_BUILD_LIBRARY_TYPE} library..."
	"[${LUA_VANILLA_VERSION}][${LUA_VERSION}][${LUA_BUILD_LIBRARY_TYPE}]")

# Get Hashes to use for download
set(LUA_VANILLA_SHA1 ${LUA_VANILLA_SHA1_${LUA_VANILLA_VERSION}})
set(LUA_VANILLA_MD5 ${LUA_VANILLA_MD5_${LUA_VANILLA_VERSION}})

if (LUA_VANILLA_MD5)
	set(LUA_VANILLA_DOWNLOAD_MD5_COMMAND URL_MD5 ${LUA_VANILLA_MD5})
else ()
	set(LUA_VANILLA_DOWNLOAD_MD5_COMMAND "")
endif()
if (LUA_VANILLA_SHA1)
	set(LUA_VANILLA_DOWNLOAD_SHA1_COMMAND URL_HASH SHA1=${LUA_VANILLA_SHA1})
else ()
	set(LUA_VANILLA_DOWNLOAD_SHA1_COMMAND "")
endif()

# # # Makefile and self-build configurations

# # Potential compiler variables
if (MSVC)
	set(LUA_VANILLA_LUA_LUAC_COMPILER_OPTIONS "/W1")
	if (BUILD_LUA_AS_DLL)
		set(LUA_VANILLA_DLL_DEFINE LUA_BUILD_AS_DLL)
	else()
		set(LUA_VANILLA_DLL_DEFINE "")
	endif()
else()
	set(LUA_VANILLA_LUALIB_COMPILER_OPTIONS "")
	set(LUA_VANILLA_DLL_DEFINE "")
endif()

# # Source files for natural build, if we have to go that far
# retrieve source files
if (LUA_VANILLA_VERSION MATCHES "^5\\.1")
	set(LUA_VANILLA_LIB_SOURCES lapi.c lcode.c ldebug.c ldo.c ldump.c lfunc.c 
		lgc.c llex.c lmem.c lobject.c lopcodes.c lparser.c lstate.c 
		lstring.c ltable.c ltm.c lundump.c lvm.c lzio.c lauxlib.c 
		lbaselib.c ldblib.c liolib.c lmathlib.c loslib.c ltablib.c 
		lstrlib.c loadlib.c linit.c)
	set(LUA_VANILLA_LUA_SOURCES lua.c )
	set(LUA_VANILLA_LUAC_SOURCES luac.c print.c )
	set(LUA_VANILLA_GENERATE_LUA_HPP true)
elseif (LUA_VANILLA_VERSION MATCHES "^5\\.2")
	set(LUA_VANILLA_LIB_SOURCES lapi.c lcode.c lctype.c ldebug.c ldo.c ldump.c 
		lfunc.c lgc.c llex.c lmem.c lobject.c lopcodes.c lparser.c 
		lstate.c lstring.c ltable.c ltm.c lundump.c lvm.c lzio.c
		lauxlib.c lbaselib.c lbitlib.c lcorolib.c ldblib.c liolib.c 
		lmathlib.c loslib.c lstrlib.c ltablib.c loadlib.c linit.c)
	set(LUA_VANILLA_LUA_SOURCES lua.c )
	set(LUA_VANILLA_LUAC_SOURCES luac.c )
	set(LUA_VANILLA_GENERATE_LUA_HPP false)
else()
	if (NOT LUA_VANILLA_VERSION MATCHES "^5\\.3")
		message(STATUS "Using the Lua 5.3 sources list for a version of Lua that is not 5.3: may result in an incomplete build or errors later")
	endif()
	set(LUA_VANILLA_LIB_SOURCES lapi.c lcode.c lctype.c ldebug.c ldo.c ldump.c 
		lfunc.c lgc.c llex.c lmem.c lobject.c lopcodes.c lparser.c lstate.c 
		lstring.c ltable.c ltm.c lundump.c lvm.c lzio.c lauxlib.c 
		lbaselib.c lbitlib.c lcorolib.c ldblib.c liolib.c lmathlib.c 
		loslib.c lstrlib.c ltablib.c lutf8lib.c loadlib.c linit.c)
	set(LUA_VANILLA_LUA_SOURCES lua.c )
	set(LUA_VANILLA_LUAC_SOURCES luac.c )
	set(LUA_VANILLA_GENERATE_LUA_HPP false)
endif()

set(LUA_VANILLA_SOURCE_DIR "${LUA_BUILD_TOPLEVEL}/src")
prepend(LUA_VANILLA_LIB_SOURCES "${LUA_VANILLA_SOURCE_DIR}/" ${LUA_VANILLA_LIB_SOURCES})
prepend(LUA_VANILLA_LUA_SOURCES "${LUA_VANILLA_SOURCE_DIR}/" ${LUA_VANILLA_LUA_SOURCES})
prepend(LUA_VANILLA_LUAC_SOURCES "${LUA_VANILLA_SOURCE_DIR}/" ${LUA_VANILLA_LUAC_SOURCES})

# download, just for the sake of download + extract
# have to use 2 different commands just to have an empty command
# that results in nothing being run
# TODO: talk to smarter CMake people...?

ExternalProject_Add(LUA_VANILLA
	BUILD_IN_SOURCE TRUE
	BUILD_ALWAYS FALSE
	TLS_VERIFY TRUE
	PREFIX ${LUA_BUILD_TOPLEVEL}
	SOURCE_DIR ${LUA_BUILD_TOPLEVEL}
	DOWNLOAD_DIR ${LUA_BUILD_TOPLEVEL}
	TMP_DIR "${LUA_BUILD_TOPLEVEL}-tmp"
	STAMP_DIR "${LUA_BUILD_TOPLEVEL}-stamp"
	INSTALL_DIR "${LUA_BUILD_INSTALL_DIR}"
	URL https://www.lua.org/ftp/lua-${LUA_VANILLA_VERSION}.tar.gz
	URL_MD5 ${LUA_VANILLA_MD5}
	URL_HASH SHA1=${LUA_VANILLA_SHA1}
	CONFIGURE_COMMAND ""
	BUILD_COMMAND ""
	INSTALL_COMMAND ""
	TEST_COMMAND ""
	BUILD_BYPRODUCTS "${LUA_VANILLA_LIB_SOURCES}" "${LUA_VANILLA_LUA_SOURCES}" "${LUA_VANILLA_LUAC_SOURCES}")

# make a quick lua.hpp for 5.1 targets that don't have it
if (LUA_VANILLA_GENERATE_LUA_HPP)
	set(LUA_VANILLA_LUA_HPP_CONTENT "// lua.hpp
// Lua header files for C++
// <<extern \"C\">> not supplied automatically because Lua also compiles as C++

extern \"C\" {
#include \"lua.h\"
#include \"lualib.h\"
#include \"lauxlib.h\"
}
")
	set(LUA_VANILLA_SOURCE_LUA_HPP "${LUA_BUILD_TOPLEVEL}-tmp/lua.hpp")
	set(LUA_VANILLA_DESTINATION_LUA_HPP "${LUA_VANILLA_SOURCE_DIR}/lua.hpp")
	file(WRITE "${LUA_VANILLA_SOURCE_LUA_HPP}" "${LUA_VANILLA_LUA_HPP_CONTENT}")
	file(TO_NATIVE_PATH "${LUA_VANILLA_SOURCE_LUA_HPP}" LUA_VANILLA_SOURCE_LUA_HPP)
	file(TO_NATIVE_PATH "${LUA_VANILLA_DESTINATION_LUA_HPP}" LUA_VANILLA_DESTINATION_LUA_HPP)
	ExternalProject_Add_Step(LUA_VANILLA
		prebuild
		# after download, before build
		DEPENDEES download
		DEPENDERS build
		BYPRODUCTS "${LUA_VANILLA_DESTINATION_LUA_HPP}"
		COMMENT "Moving \"${LUA_VANILLA_SOURCE_LUA_HPP}\" to \"${LUA_VANILLA_DESTINATION_LUA_HPP}\"..."
		COMMAND "${CMAKE_COMMAND}" -E copy "${LUA_VANILLA_SOURCE_LUA_HPP}" "${LUA_VANILLA_DESTINATION_LUA_HPP}")
endif()

# # Target names
set(liblua "liblua-${LUA_VANILLA_VERSION}")
set(luainterpreter "lua-${LUA_VANILLA_VERSION}")
set(luacompiler "luac-${LUA_VANILLA_VERSION}")

# Lua does not out-of-the-box support building 
# a shared library: http://lua-users.org/lists/lua-l/2006-10/msg00098.html
# in this case, we essentially need to
# build Lua for our purposes, which is annoying
# the external project is just a dummy to get and extract the file:
# after that, we define actual targets...!

# make an actual, buildable target
# that other parts of the code can depend on
add_library(${liblua} ${LUA_BUILD_LIBRARY_TYPE} "${LUA_VANILLA_LIB_SOURCES}")
set_target_properties(${liblua}
	PROPERTIES
	LANGUAGE C
	LINKER_LANGUAGE C
	C_STANDARD 99
	C_EXTENSIONS TRUE
	POSITION_INDEPENDENT_CODE TRUE
	INCLUDE_DIRECTORIES ${LUA_VANILLA_SOURCE_DIR}
	OUTPUT_NAME ${LUA_BUILD_LIBNAME}
	RUNTIME_OUTPUT_NAME ${LUA_BUILD_LIBNAME}
	LIBRARY_OUTPUT_NAME ${LUA_BUILD_LIBNAME}
	ARCHIVE_OUTPUT_NAME ${LUA_BUILD_LIBNAME})
target_include_directories(${liblua}
	PUBLIC ${LUA_VANILLA_SOURCE_DIR})
target_compile_definitions(${liblua}
	PUBLIC LUA_COMPAT_ALL ${LUA_VANILLA_DLL_DEFINE})
if (MSVC)
	target_compile_options(${liblua}
		PRIVATE /W1)
else()
	target_compile_options(${liblua}
		PRIVATE -w)
endif()
if (WIN32)
	#target_compile_definitions(${liblua}
	#	PRIVATE LUA_USE_WINDOWS)
else()
	target_compile_definitions(${liblua} 
		PRIVATE LUA_USE_LINUX)
endif()
target_compile_options(${liblua}
	PRIVATE ${LUA_VANILLA_LUALIB_COMPILER_OPTIONS})
add_dependencies(${liblua} LUA_VANILLA)
if (CMAKE_DL_LIBS)
	target_link_libraries(${liblua} ${CMAKE_DL_LIBS})
endif()
if (UNIX)
	target_link_libraries(${liblua} m)
endif()

# we don't really need this section...
# Lua Interpreter
add_executable(${luainterpreter} ${LUA_VANILLA_LUA_SOURCES})
set_target_properties(${luainterpreter}
	PROPERTIES
	LANGUAGE C
	LINKER_LANGUAGE C
	C_STANDARD 99
	C_EXTENSIONS TRUE
	OUTPUT_NAME lua-${LUA_VANILLA_VERSION})
target_include_directories(${luainterpreter}
	PRIVATE ${LUA_VANILLA_SOURCE_DIR})
target_compile_definitions(${luainterpreter}
	PUBLIC LUA_COMPAT_ALL ${LUA_VANILLA_DLL_DEFINE}
	PRIVATE LUA_COMPAT_ALL ${LUA_VANILLA_DLL_DEFINE})
if (MSVC)
	target_compile_options(${luainterpreter}
		PRIVATE /W1)
else()
	target_compile_options(${luainterpreter}
		PRIVATE -w)
endif()
if (WIN32)
	#target_compile_definitions(${luainterpreter} 
	#	PRIVATE LUA_USE_WINDOWS)
else()
	target_compile_definitions(${luainterpreter} 
		PRIVATE LUA_USE_LINUX)
endif()
target_compile_options(${luainterpreter}
	PRIVATE ${LUA_VANILLA_LUA_LUAC_COMPILER_OPTIONS})
target_link_libraries(${luainterpreter} ${liblua})
if (CMAKE_DL_LIBS)
	target_link_libraries(${luainterpreter} ${CMAKE_DL_LIBS})
endif()
if (UNIX)
	target_link_libraries(${luainterpreter} m readline)
endif()

# LuaC Compiler
add_executable(${luacompiler} ${LUA_VANILLA_LUAC_SOURCES})
if (BUILD_LUA_AS_DLL)
	# TODO: figure out how to make DLL internal symbols for lua public so we don't have to do this
	target_sources(${luacompiler} PRIVATE ${LUA_VANILLA_LIB_SOURCES})
endif()
set_target_properties(${luacompiler}
	PROPERTIES
	LANGUAGE C
	LINKER_LANGUAGE C
	C_STANDARD 99
	C_EXTENSIONS TRUE
	OUTPUT_NAME luac-${LUA_VANILLA_VERSION})
target_include_directories(${luacompiler}
	PRIVATE ${LUA_VANILLA_SOURCE_DIR})
target_compile_options(${luacompiler}
	PRIVATE ${LUA_VANILLA_LUA_LUAC_COMPILER_OPTIONS})
target_compile_definitions(${luacompiler}
	PUBLIC LUA_COMPAT_ALL ${LUA_VANILLA_DLL_DEFINE}
	PRIVATE LUA_COMPAT_ALL ${LUA_VANILLA_DLL_DEFINE})
if (MSVC)
	target_compile_options(${luacompiler}
		PRIVATE /W1)
else()
	target_compile_options(${luacompiler}
		PRIVATE -w)
endif()
if (WIN32)
	#target_compile_definitions(${luacompiler} 
	#	PRIVATE LUA_USE_WINDOWS)
else()
	target_compile_definitions(${luacompiler} 
		PRIVATE LUA_USE_LINUX)
endif()
target_link_libraries(${luacompiler} ${liblua})
if (CMAKE_DL_LIBS)
	target_link_libraries(${luacompiler} ${CMAKE_DL_LIBS})
endif()
if (UNIX)
	# TODO: make readline optional?
	target_link_libraries(${luacompiler} m readline)
endif()

# set externally-visible target indicator
set(LUA_LIBRARIES ${liblua})
set(LUA_INTERPRETER ${luainterpreter})
set(LUA_COMPILER ${luacompiler})
set(LUA_INCLUDE_DIRS "${LUA_VANILLA_SOURCE_DIR}")
