cmake_minimum_required(VERSION 2.6)
project(OSRM)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
include(FindPackageHandleStandardArgs)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(GetGitRevisionDescription)
git_describe(GIT_DESCRIPTION)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

set(bitness 32)
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(bitness 64)
  message(STATUS "Building on a 64 bit system")
else()
  message(WARNING "Building on a 32 bit system is unsupported")
endif()

add_custom_command(OUTPUT ${CMAKE_SOURCE_DIR}/Util/UUID.cpp UUID.cpp.alwaysbuild
  COMMAND ${CMAKE_COMMAND} -DSOURCE_DIR=${CMAKE_SOURCE_DIR}
    -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/UUID-Config.cmake
  DEPENDS
    ${CMAKE_SOURCE_DIR}/Util/UUID.cpp.in
  	${CMAKE_SOURCE_DIR}/cmake/UUID-Config.cmake
  COMMENT "Configuring UUID.cpp"
  VERBATIM)

add_custom_target(UUIDConfigure DEPENDS ${CMAKE_SOURCE_DIR}/Util/UUID.cpp )

set(BOOST_COMPONENTS filesystem iostreams program_options regex system thread)

configure_file(Util/GitDescription.cpp.in ${CMAKE_SOURCE_DIR}/Util/GitDescription.cpp)
file(GLOB ExtractorGlob Extractor/*.cpp)
set(ExtractorSources extractor.cpp ${ExtractorGlob})
add_executable(osrm-extract ${ExtractorSources} )

file( GLOB PrepareGlob Contractor/*.cpp DataStructures/HilbertValue.cpp)
set( PrepareSources prepare.cpp ${PrepareGlob} )
add_executable( osrm-prepare ${PrepareSources} )

file(GLOB ServerGlob Server/*.cpp)
file(GLOB DescriptorGlob Descriptors/*.cpp)
file(GLOB DatastructureGlob DataStructures/SearchEngineData.cpp)
file(GLOB CoordinateGlob DataStructures/Coordinate.cpp)
file(GLOB AlgorithmGlob Algorithms/*.cpp)
file(GLOB HttpGlob Server/Http/*.cpp)
file(GLOB LibOSRMGlob Library/*.cpp)

set(OSRMSources ${LibOSRMGlob} ${DescriptorGlob} ${DatastructureGlob} ${CoordinateGlob} ${AlgorithmGlob} ${HttpGlob})
add_library( OSRM SHARED ${OSRMSources} Util/GitDescription.cpp Util/UUID.cpp )
add_library( UUID STATIC Util/UUID.cpp )
add_library( GITDESCRIPTION STATIC Util/GitDescription.cpp )
add_dependencies( UUID UUIDConfigure )
add_dependencies( GITDESCRIPTION GIT_DESCRIPTION )

add_executable(osrm-routed routed.cpp ${ServerGlob})
set_target_properties(osrm-routed PROPERTIES COMPILE_FLAGS -DROUTED)
add_executable(osrm-datastore datastore.cpp ${CoordinateGlob})

# Check the release mode
if(NOT CMAKE_BUILD_TYPE MATCHES Debug)
	set(CMAKE_BUILD_TYPE Release)
endif(NOT CMAKE_BUILD_TYPE MATCHES Debug)
if(CMAKE_BUILD_TYPE MATCHES Debug)
    message(STATUS "Configuring OSRM in debug mode")
endif(CMAKE_BUILD_TYPE MATCHES Debug)
if(CMAKE_BUILD_TYPE MATCHES Release)
    message(STATUS "Configuring OSRM in release mode")
endif(CMAKE_BUILD_TYPE MATCHES Release)

# Configuring compilers
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
	# using Clang
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-unknown-pragmas -Wno-unneeded-internal-declaration -pedantic -fPIC")
	message(STATUS "OpenMP parallelization not available using clang++")
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
	# using GCC
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -fopenmp -pedantic -fPIC")
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
	# using Intel C++
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-intel -wd10237 -Wall -openmp -ipo -fPIC")
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
	# using Visual Studio C++
endif()

# Configuring other platform dependencies
if(APPLE)
	SET(CMAKE_OSX_ARCHITECTURES "x86_64")
	message(STATUS "Set Architecture to x64 on OS X")
    EXEC_PROGRAM(uname ARGS -v  OUTPUT_VARIABLE DARWIN_VERSION)
    STRING(REGEX MATCH "[0-9]+" DARWIN_VERSION ${DARWIN_VERSION})
    if(DARWIN_VERSION GREATER 12)
        MESSAGE(STATUS "Activating -std=c++11 flag for >= OS X 10.9")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -stdlib=libc++")
    endif(DARWIN_VERSION GREATER 12)
endif()

if(UNIX AND NOT APPLE)
    target_link_libraries( osrm-datastore rt )
    target_link_libraries( OSRM rt )
endif(UNIX AND NOT APPLE)

#Check Boost
set(BOOST_MIN_VERSION "1.46.0")
find_package( Boost ${BOOST_MIN_VERSION} COMPONENTS ${BOOST_COMPONENTS} REQUIRED )
if(NOT Boost_FOUND)
      message(FATAL_ERROR "Fatal error: Boost (version >= 1.46.0) required.\n")
endif(NOT Boost_FOUND)
include_directories(${Boost_INCLUDE_DIRS})

target_link_libraries( OSRM ${Boost_LIBRARIES} )
target_link_libraries( osrm-extract ${CoordinateGlob} ${Boost_LIBRARIES} UUID GITDESCRIPTION )
target_link_libraries( osrm-prepare ${CoordinateGlob} ${Boost_LIBRARIES} UUID GITDESCRIPTION )
target_link_libraries( osrm-routed ${Boost_LIBRARIES} OSRM UUID GITDESCRIPTION )
target_link_libraries( osrm-datastore ${Boost_LIBRARIES} UUID GITDESCRIPTION )

find_package( Threads REQUIRED )
target_link_libraries (osrm-extract ${CMAKE_THREAD_LIBS_INIT})

find_package( Lua52 )
if( NOT LUA52_FOUND )
  find_package( Lua51 REQUIRED )
  if(NOT APPLE)
    find_package( LuaJIT 5.1 )
  endif( NOT APPLE )
else( NOT LUA52_FOUND )
  if(NOT APPLE)
    find_package( LuaJIT 5.2 )
  endif(NOT APPLE)
endif( NOT LUA52_FOUND )

if( LUAJIT_FOUND )
  target_link_libraries( osrm-extract ${LUAJIT_LIBRARIES} )
  target_link_libraries( osrm-prepare ${LUAJIT_LIBRARIES} )
else()
  target_link_libraries( osrm-extract ${LUA_LIBRARY} )
  target_link_libraries( osrm-prepare ${LUA_LIBRARY} )
endif()
include_directories(${LUA_INCLUDE_DIR})

find_package( LibXml2 REQUIRED )
include_directories(${LIBXML2_INCLUDE_DIR})
target_link_libraries (osrm-extract ${LIBXML2_LIBRARIES})

find_package( Luabind REQUIRED )
include_directories(${LUABIND_INCLUDE_DIR})
target_link_libraries (osrm-extract ${LUABIND_LIBRARY})
target_link_libraries (osrm-prepare ${LUABIND_LIBRARY})

find_package( STXXL REQUIRED )
include_directories(${STXXL_INCLUDE_DIR})
target_link_libraries (OSRM ${STXXL_LIBRARY})
target_link_libraries (osrm-extract ${STXXL_LIBRARY})
target_link_libraries (osrm-prepare ${STXXL_LIBRARY})

find_package( OSMPBF REQUIRED )
include_directories(${OSMPBF_INCLUDE_DIR})
target_link_libraries (osrm-extract ${OSMPBF_LIBRARY})
target_link_libraries (osrm-prepare ${OSMPBF_LIBRARY})

find_package( Protobuf REQUIRED )
include_directories(${PROTOBUF_INCLUDE_DIRS})
target_link_libraries (osrm-extract ${PROTOBUF_LIBRARY})
target_link_libraries (osrm-prepare ${PROTOBUF_LIBRARY})

find_package ( BZip2 REQUIRED )
include_directories(${BZIP_INCLUDE_DIRS})
target_link_libraries (osrm-extract ${BZIP2_LIBRARIES})

find_package( ZLIB REQUIRED )
include_directories(${ZLIB_INCLUDE_DIRS})
target_link_libraries (osrm-extract ${ZLIB_LIBRARY})
target_link_libraries (osrm-routed ${ZLIB_LIBRARY})

if(WITH_TOOLS)
	message(STATUS "Activating OSRM internal tools")
	find_package( GDAL )
	if(GDAL_FOUND)
		add_executable(osrm-components Tools/componentAnalysis.cpp)
		include_directories(${GDAL_INCLUDE_DIR})
		target_link_libraries(
			osrm-components ${GDAL_LIBRARIES} ${Boost_LIBRARIES} UUID GITDESCRIPTION ${CoordinateGlob}
		)
	endif(GDAL_FOUND)
	add_executable ( osrm-cli Tools/simpleclient.cpp)
	target_link_libraries( osrm-cli ${Boost_LIBRARIES} OSRM UUID GITDESCRIPTION)
    add_executable ( osrm-io-benchmark Tools/io-benchmark.cpp )
    target_link_libraries( osrm-io-benchmark ${Boost_LIBRARIES} GITDESCRIPTION)
    add_executable ( osrm-unlock-all Tools/unlock_all_mutexes.cpp )
    target_link_libraries( osrm-unlock-all ${Boost_LIBRARIES} GITDESCRIPTION)
    if(UNIX AND NOT APPLE)
        target_link_libraries( osrm-unlock-all rt )
    endif(UNIX AND NOT APPLE)
endif(WITH_TOOLS)

install (TARGETS osrm-extract DESTINATION bin)
install (TARGETS osrm-prepare DESTINATION bin)
install (TARGETS osrm-datastore DESTINATION bin)
install (TARGETS osrm-routed DESTINATION bin)
install (TARGETS OSRM DESTINATION lib)
