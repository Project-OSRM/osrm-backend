cmake_minimum_required(VERSION 2.6)
project(OSRM)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
set(BOOST_COMPONENTS filesystem regex system thread)

file(GLOB ExtractorGlob Extractor/*.cpp)
set(ExtractorSources extractor.cpp ${ExtractorGlob})
add_executable(osrm-extract ${ExtractorSources})

file(GLOB PrepareGlob Contractor/*.cpp)
set(PrepareSources createHierarchy.cpp ${PrepareGlob})
add_executable(osrm-prepare ${PrepareSources})

file(GLOB RoutedGlob Server/DataStructures/*.cpp Descriptors/*.cpp DataStructures/*.cpp)
set(RoutedSources routed.cpp ${RoutedGlob})
add_executable(osrm-routed ${RoutedSources})
set_target_properties(osrm-routed PROPERTIES COMPILE_FLAGS -DROUTED)

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

#Configuring compilers
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
	# using Clang
	set(CMAKE_CXX_FLAGS "-Wall -Wno-unknown-pragmas")
	message(STATUS "OpenMP parallelization not available using clang++")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
	# using GCC
	set(CMAKE_CXX_FLAGS "-Wall -fopenmp") 
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
	# using Intel C++
	set(CMAKE_CXX_FLAGS "-static-intel -wd10237 -Wall -openmp -ipo")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
	# using Visual Studio C++
endif()

if(APPLE)
	SET(CMAKE_OSX_ARCHITECTURES "x86_64")
	message("Set Archtitecture to x64 on OS X")
endif()

#Check Boost
set(BOOST_MIN_VERSION "1.44.0")
find_package( Boost ${BOOST_MIN_VERSION} COMPONENTS ${BOOST_COMPONENTS} REQUIRED )
if (NOT Boost_FOUND)
      message(FATAL_ERROR "Fatal error: Boost (version >= 1.44.0) required.\n")
endif (NOT Boost_FOUND)
target_link_libraries( osrm-extract ${Boost_LIBRARIES} )
target_link_libraries( osrm-prepare ${Boost_LIBRARIES} )
target_link_libraries( osrm-routed ${Boost_LIBRARIES} )

find_package ( BZip2 REQUIRED )
include_directories(${BZIP_INCLUDE_DIRS})
target_link_libraries (osrm-extract ${BZIP2_LIBRARIES})

find_package( ZLIB REQUIRED )
target_link_libraries (osrm-extract ${ZLIB_LIBRARY})
target_link_libraries (osrm-routed ${ZLIB_LIBRARY})

find_package( Threads REQUIRED )
target_link_libraries (osrm-extract ${Threads_LIBRARY})

find_package( Lua51 REQUIRED ) 
include_directories(${LUA_INCLUDE_DIR})
target_link_libraries( osrm-extract ${LUA_LIBRARY} )
target_link_libraries( osrm-prepare ${LUA_LIBRARY} )

find_package( LibXml2 REQUIRED )
include_directories(${LIBXML2_INCLUDE_DIR})
target_link_libraries (osrm-extract ${LIBXML2_LIBRARIES})

find_package( Luabind REQUIRED )
include_directories(${LUABIND_INCLUDE_DIR})
target_link_libraries (osrm-extract ${LUABIND_LIBRARY})
target_link_libraries (osrm-prepare ${LUABIND_LIBRARY})

find_package( Protobuf REQUIRED )
include_directories(${PROTOBUF_INCLUDE_DIRS})
target_link_libraries (osrm-extract ${PROTOBUF_LIBRARY})
target_link_libraries (osrm-prepare ${PROTOBUF_LIBRARY})

find_package( STXXL REQUIRED )
include_directories(${STXXL_INCLUDE_DIR})
target_link_libraries (osrm-extract ${STXXL_LIBRARY})
target_link_libraries (osrm-prepare ${STXXL_LIBRARY})

find_package( OSMPBF REQUIRED )
include_directories(${OSMPBF_INCLUDE_DIR})
target_link_libraries (osrm-extract ${OSMPBF_LIBRARY})
target_link_libraries (osrm-prepare ${OSMPBF_LIBRARY})

if(WITH_TOOLS)
	message("-- Activating OSRM internal tools")
	find_package( GDAL )
	if(GDAL_FOUND)
		add_executable(osrm-components Tools/componentAnalysis.cpp)
		include_directories(${GDAL_INCLUDE_DIR})
		target_link_libraries( osrm-components ${GDAL_LIBRARIES} )
		target_link_libraries( osrm-components ${Boost_LIBRARIES} )
	endif(GDAL_FOUND)
endif(WITH_TOOLS)
