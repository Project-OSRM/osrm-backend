cmake_minimum_required(VERSION 2.8)
project(OSRM)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
include(CheckCXXCompilerFlag)
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

include_directories(${CMAKE_SOURCE_DIR}/Include/)

add_custom_command(OUTPUT ${CMAKE_SOURCE_DIR}/Util/UUID.cpp UUID.cpp.alwaysbuild
  COMMAND ${CMAKE_COMMAND} -DSOURCE_DIR=${CMAKE_SOURCE_DIR}
    -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/UUID-Config.cmake
  DEPENDS
    ${CMAKE_SOURCE_DIR}/Util/UUID.cpp.in
    ${CMAKE_SOURCE_DIR}/cmake/UUID-Config.cmake
  COMMENT "Configuring UUID.cpp"
  VERBATIM)

add_custom_target(UUIDConfigure DEPENDS ${CMAKE_SOURCE_DIR}/Util/UUID.cpp)

set(BOOST_COMPONENTS date_time filesystem iostreams program_options regex system thread)

configure_file(
  ${CMAKE_SOURCE_DIR}/Util/GitDescription.cpp.in
  ${CMAKE_SOURCE_DIR}/Util/GitDescription.cpp
)
file(GLOB ExtractorGlob Extractor/*.cpp)
set(ExtractorSources extractor.cpp ${ExtractorGlob})
add_executable(osrm-extract ${ExtractorSources})

file(GLOB PrepareGlob Contractor/*.cpp DataStructures/HilbertValue.cpp DataStructures/RestrictionMap.cpp)
set(PrepareSources prepare.cpp ${PrepareGlob})
add_executable(osrm-prepare ${PrepareSources})

file(GLOB ServerGlob Server/*.cpp)
file(GLOB DescriptorGlob Descriptors/*.cpp)
file(GLOB DatastructureGlob DataStructures/SearchEngineData.cpp DataStructures/RouteParameters.cpp)
file(GLOB CoordinateGlob DataStructures/Coordinate.cpp)
file(GLOB AlgorithmGlob Algorithms/*.cpp)
file(GLOB HttpGlob Server/Http/*.cpp)
file(GLOB LibOSRMGlob Library/*.cpp)

set(
  OSRMSources
  ${LibOSRMGlob}
  ${DescriptorGlob}
  ${DatastructureGlob}
  ${CoordinateGlob}
  ${AlgorithmGlob}
  ${HttpGlob}
)
add_library(COORDLIB STATIC ${CoordinateGlob})
add_library(OSRM ${OSRMSources} Util/GitDescription.cpp Util/UUID.cpp)
add_library(UUID STATIC Util/UUID.cpp)
add_library(GITDESCRIPTION STATIC Util/GitDescription.cpp)
add_dependencies(UUID UUIDConfigure)
add_dependencies(GITDESCRIPTION GIT_DESCRIPTION)

add_executable(osrm-routed routed.cpp ${ServerGlob})
set_target_properties(osrm-routed PROPERTIES COMPILE_FLAGS -DROUTED)
add_executable(osrm-datastore datastore.cpp)

# Check the release mode
if(NOT CMAKE_BUILD_TYPE MATCHES Debug)
  set(CMAKE_BUILD_TYPE Release)
endif()
if(CMAKE_BUILD_TYPE MATCHES Debug)
  message(STATUS "Configuring OSRM in debug mode")
  if(NOT "${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    message(STATUS "adding profiling flags")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage -fno-inline")
    set(CMAKE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fprofile-arcs -ftest-coverage -fno-inline")
  endif()
endif()
if(CMAKE_BUILD_TYPE MATCHES Release)
  message(STATUS "Configuring OSRM in release mode")
  # Check if LTO is available
  set(LTO_FLAGS "")
  CHECK_CXX_COMPILER_FLAG("-flto" HAS_LTO_FLAG)
  if (HAS_LTO_FLAG)
    set(LTO_FLAGS "${LTO_FLAGS} -flto")

    # Since gcc 4.9 the LTO format is non-standart ('slim'), so we need to use the build-in tools
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" AND
        NOT "${CMAKE_CXX_COMPILER_VERSION}" VERSION_LESS "4.9.0")
      message(STATUS "Using gcc specific binutils for LTO.")
      set(CMAKE_AR     "/usr/bin/gcc-ar")
      set(CMAKE_RANLIB "/usr/bin/gcc-ranlib")
    endif()
  endif (HAS_LTO_FLAG)
endif()

# Configuring compilers
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  # using Clang
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wunreachable-code -Wno-unknown-pragmas -Wno-unneeded-internal-declaration -pedantic -fPIC")
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

# disable partitioning of LTO process when possible (fixes Debian issues)
set(LTO_PARTITION_FLAGS "")
CHECK_CXX_COMPILER_FLAG("-flto-partition=none" HAS_LTO_PARTITION_FLAG)
if    (HAS_LTO_PARTITION_FLAG)
  set(LTO_PARTITION_FLAGS "${LTO_PARTITION_FLAGS} -flto-partition=none")
endif (HAS_LTO_PARTITION_FLAG)

# Add Link-Time-Optimization flags, if supported (GCC >= 4.7) and enabled
set(CMAKE_CXX_FLAGS           "${CMAKE_CXX_FLAGS}           ${LTO_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS    "${CMAKE_EXE_LINKER_FLAGS}    ${LTO_FLAGS} ${LTO_PARTITION_FLAGS}")
set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} ${LTO_FLAGS} ${LTO_PARTITION_FLAGS}")

# Activate C++11
ADD_DEFINITIONS(
    -std=c++11 # Or -std=c++0x
    # Other flags
)

# Configuring other platform dependencies
if(APPLE)
  set(CMAKE_OSX_ARCHITECTURES "x86_64")
  message(STATUS "Set Architecture to x64 on OS X")
  exec_program(uname ARGS -v  OUTPUT_VARIABLE DARWIN_VERSION)
  string(REGEX MATCH "[0-9]+" DARWIN_VERSION ${DARWIN_VERSION})
#  if(DARWIN_VERSION GREATER 12 AND NOT OSXLIBSTD)
#    message(STATUS "Activating -std=c++11 flag for >= OS X 10.9")
#    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
#  endif()
  if(OSXLIBSTD)
    message(STATUS "linking against ${OSXLIBSTD}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=${OSXLIBSTD}")
  elseif(DARWIN_VERSION GREATER 12)
    message(STATUS "linking against libc++")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
  endif()
endif()

if(UNIX AND NOT APPLE)
  target_link_libraries(osrm-datastore rt)
  target_link_libraries(OSRM rt)
endif()

#Check Boost
set(BOOST_MIN_VERSION "1.49.0")
find_package(Boost ${BOOST_MIN_VERSION} COMPONENTS ${BOOST_COMPONENTS} REQUIRED)
if(NOT Boost_FOUND)
  message(FATAL_ERROR "Fatal error: Boost (version >= 1.49.0) required.\n")
endif()
include_directories(${Boost_INCLUDE_DIRS})

target_link_libraries(OSRM ${Boost_LIBRARIES} COORDLIB)
target_link_libraries(osrm-extract ${Boost_LIBRARIES} UUID GITDESCRIPTION COORDLIB)
target_link_libraries(osrm-prepare ${Boost_LIBRARIES} UUID GITDESCRIPTION COORDLIB)
target_link_libraries(osrm-routed ${Boost_LIBRARIES} OSRM UUID GITDESCRIPTION)
target_link_libraries(osrm-datastore ${Boost_LIBRARIES} UUID GITDESCRIPTION COORDLIB)

find_package(Threads REQUIRED)
target_link_libraries(osrm-extract ${CMAKE_THREAD_LIBS_INIT})

find_package(Lua52)
if(NOT LUA52_FOUND)
  find_package(Lua51 REQUIRED)
  if(NOT APPLE)
    find_package(LuaJIT 5.1)
  endif()
else()
  if(NOT APPLE)
    find_package(LuaJIT 5.2)
  endif()
endif()

if( LUAJIT_FOUND )
  target_link_libraries(osrm-extract ${LUAJIT_LIBRARIES})
  target_link_libraries(osrm-prepare ${LUAJIT_LIBRARIES})
else()
  target_link_libraries(osrm-extract ${LUA_LIBRARY})
  target_link_libraries(osrm-prepare ${LUA_LIBRARY})
endif()
include_directories(${LUA_INCLUDE_DIR})

find_package(LibXml2 REQUIRED)
include_directories(${LIBXML2_INCLUDE_DIR})
target_link_libraries(osrm-extract ${LIBXML2_LIBRARIES})

find_package( Luabind REQUIRED )
include_directories(${LUABIND_INCLUDE_DIR})
target_link_libraries(osrm-extract ${LUABIND_LIBRARY})
target_link_libraries(osrm-prepare ${LUABIND_LIBRARY})

find_package( STXXL REQUIRED )
include_directories(${STXXL_INCLUDE_DIR})
target_link_libraries(OSRM ${STXXL_LIBRARY})
target_link_libraries(osrm-extract ${STXXL_LIBRARY})
target_link_libraries(osrm-prepare ${STXXL_LIBRARY})

find_package( OSMPBF REQUIRED )
include_directories(${OSMPBF_INCLUDE_DIR})
target_link_libraries(osrm-extract ${OSMPBF_LIBRARY})
target_link_libraries(osrm-prepare ${OSMPBF_LIBRARY})

find_package(Protobuf REQUIRED)
include_directories(${PROTOBUF_INCLUDE_DIRS})
target_link_libraries(osrm-extract ${PROTOBUF_LIBRARY})
target_link_libraries(osrm-prepare ${PROTOBUF_LIBRARY})

find_package(BZip2 REQUIRED)
include_directories(${BZIP_INCLUDE_DIRS})
target_link_libraries(osrm-extract ${BZIP2_LIBRARIES})

find_package(ZLIB REQUIRED)
include_directories(${ZLIB_INCLUDE_DIRS})
target_link_libraries(osrm-extract ${ZLIB_LIBRARY})
target_link_libraries(osrm-routed ${ZLIB_LIBRARY})

if(WITH_TOOLS)
  message(STATUS "Activating OSRM internal tools")
  find_package(GDAL)
  if(GDAL_FOUND)
    add_executable(osrm-components Tools/components.cpp)
    include_directories(${GDAL_INCLUDE_DIR})
    target_link_libraries(
      osrm-components
      ${GDAL_LIBRARIES} ${Boost_LIBRARIES} UUID GITDESCRIPTION COORDLIB)
  endif()
  add_executable(osrm-cli Tools/simpleclient.cpp)
  target_link_libraries(osrm-cli ${Boost_LIBRARIES} OSRM UUID GITDESCRIPTION)
    add_executable(osrm-io-benchmark Tools/io-benchmark.cpp)
    target_link_libraries(osrm-io-benchmark ${Boost_LIBRARIES} GITDESCRIPTION)
    add_executable(osrm-unlock-all Tools/unlock_all_mutexes.cpp)
    target_link_libraries(osrm-unlock-all ${Boost_LIBRARIES} GITDESCRIPTION)
    if(UNIX AND NOT APPLE)
        target_link_libraries(osrm-unlock-all rt)
    endif()
endif()

file(GLOB InstallGlob Include/osrm/*.h Library/OSRM.h)

# Add RPATH info to executables so that when they are run after being installed
# (i.e., from /usr/local/bin/) the linker can find library dependencies. For
# more info see http://www.cmake.org/Wiki/CMake_RPATH_handling
set_property(TARGET osrm-extract PROPERTY INSTALL_RPATH_USE_LINK_PATH TRUE)
set_property(TARGET osrm-prepare PROPERTY INSTALL_RPATH_USE_LINK_PATH TRUE)
set_property(TARGET osrm-datastore PROPERTY INSTALL_RPATH_USE_LINK_PATH TRUE)
set_property(TARGET osrm-routed PROPERTY INSTALL_RPATH_USE_LINK_PATH TRUE)

install(FILES ${InstallGlob} DESTINATION include/osrm)
install(TARGETS osrm-extract DESTINATION bin)
install(TARGETS osrm-prepare DESTINATION bin)
install(TARGETS osrm-datastore DESTINATION bin)
install(TARGETS osrm-routed DESTINATION bin)
install(TARGETS OSRM DESTINATION lib)
list(GET Boost_LIBRARIES 1 BOOST_LIBRARY_FIRST)
get_filename_component(BOOST_LIBRARY_LISTING "${BOOST_LIBRARY_FIRST}" PATH)
set(BOOST_LIBRARY_LISTING "-L${BOOST_LIBRARY_LISTING}")
foreach (lib ${Boost_LIBRARIES})
  get_filename_component(BOOST_LIBRARY_NAME "${lib}" NAME_WE)
  string(REPLACE "lib" "" BOOST_LIBRARY_NAME ${BOOST_LIBRARY_NAME})
  set(BOOST_LIBRARY_LISTING "${BOOST_LIBRARY_LISTING} -l${BOOST_LIBRARY_NAME}")
endforeach ()

configure_file(${CMAKE_SOURCE_DIR}/cmake/pkgconfig.in libosrm.pc @ONLY)
install(FILES ${PROJECT_BINARY_DIR}/libosrm.pc DESTINATION lib/pkgconfig)
