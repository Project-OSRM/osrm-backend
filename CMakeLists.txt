cmake_minimum_required(VERSION 3.18)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_CURRENT_BINARY_DIR AND NOT MSVC_IDE)
  message(FATAL_ERROR "In-source builds are not allowed.
Please create a directory and run cmake from there, passing the path to this source directory as the last argument.
This process created the file `CMakeCache.txt' and the directory `CMakeFiles'. Please delete them.")
endif()

# detect if this is included as subproject and if so expose
# some variables to its parent scope
get_directory_property(BUILD_AS_SUBPROJECT PARENT_DIRECTORY)
if(BUILD_AS_SUBPROJECT)
  message(STATUS "Building libosrm as subproject.")
endif()

# set OSRM_BUILD_DIR location (might be used in various scripts)
if (NOT WIN32 AND NOT DEFINED ENV{OSRM_BUILD_DIR})
  set(ENV{OSRM_BUILD_DIR} ${CMAKE_CURRENT_BINARY_DIR})
endif()

option(BUILD_PACKAGE "Build OSRM package" OFF)
option(ENABLE_ASSERTIONS "Use assertions in release mode" OFF)
option(ENABLE_CCACHE "Speed up incremental rebuilds via ccache" ON)
option(ENABLE_CLANG_TIDY "Enables clang-tidy checks" OFF)
option(ENABLE_COVERAGE "Build with coverage instrumentalisation" OFF)
option(ENABLE_DEBUG_LOGGING "Use debug logging in release mode" OFF)
option(ENABLE_FUZZING "Fuzz testing using LLVM's libFuzzer" OFF)
option(ENABLE_LTO "Use Link Time Optimisation" ON)
option(ENABLE_NODE_BINDINGS "Build NodeJs bindings" OFF)
option(ENABLE_SANITIZER "Use memory sanitizer for Debug build" OFF)

# Dependencies are provided by vcpkg in manifest mode (see vcpkg.json).
# The vcpkg toolchain file is wired up via CMakePresets.json or by passing
# -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake.

if (ENABLE_CLANG_TIDY)
  find_program(CLANG_TIDY_COMMAND NAMES clang-tidy)
  if(NOT CLANG_TIDY_COMMAND)
    message(FATAL_ERROR "ENABLE_CLANG_TIDY is ON but clang-tidy is not found!")
  else()
    message(STATUS "Found clang-tidy at ${CLANG_TIDY_COMMAND}")
    set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_COMMAND};--warnings-as-errors=*;--header-filter=.*")
  endif()
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

project(OSRM C CXX)

if(ENABLE_LTO AND (CMAKE_BUILD_TYPE MATCHES Release OR CMAKE_BUILD_TYPE MATCHES MinRelSize OR CMAKE_BUILD_TYPE MATCHES RelWithDebInfo))
  include(CheckIPOSupported)
  check_ipo_supported(RESULT LTO_SUPPORTED OUTPUT error)
  if(LTO_SUPPORTED)
      message(STATUS "IPO / LTO enabled")
      set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
  else()
      message(FATAL_ERROR "IPO / LTO not supported: <${error}>")
  endif()
endif()

# add @loader_path/$ORIGIN to rpath to make binaries relocatable
if (APPLE)
  set(CMAKE_BUILD_RPATH "@loader_path")
elseif(NOT WIN32)
  set(CMAKE_BUILD_RPATH "\$ORIGIN")
  # https://stackoverflow.com/questions/6324131/rpath-origin-not-having-desired-effect
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-z,origin")
endif()

include(JSONParser)
file(READ "package.json" packagejsonraw)
sbeParseJson(packagejson packagejsonraw)

# This regex supports multiple semver version formats:
# - (YYYY-2000).M.patchlevel: Monthly releases, e.g., 26.4.0 (April 2026)
#   MAJOR is the year offset (year - 2000)
# - Legacy semver: traditional major.minor.patch with optional prerelease, e.g., 6.0.0, 6.0.0-rc1

# Try monthly release format: (YYYY-2000).M.patchlevel with month 1-12, no leading zeros
if (packagejson.version MATCHES "^([0-9]+)\\.(1[0-2]|[1-9])\\.(0|[1-9][0-9]*)$")
  # Monthly format: (YYYY-2000).M.patchlevel
  # MAJOR is already offset (year - 2000)
  set(OSRM_VERSION_MAJOR            ${CMAKE_MATCH_1})
  set(OSRM_VERSION_MINOR            ${CMAKE_MATCH_2})
  set(OSRM_VERSION_PATCH            ${CMAKE_MATCH_3})
  set(OSRM_VERSION_PRERELEASE_BUILD "")
  set(OSRM_VERSION "${packagejson.version}")
# Try legacy semver format (major.minor.patch with optional prerelease/build)
elseif (packagejson.version MATCHES "^([0-9]+)\\.([0-9]+)\\.([0-9]+)([-+][0-9a-zA-Z.-]+)?$")
  # Legacy semver format: major.minor.patch[-+prerelease]
  set(OSRM_VERSION_MAJOR            ${CMAKE_MATCH_1})
  set(OSRM_VERSION_MINOR            ${CMAKE_MATCH_2})
  set(OSRM_VERSION_PATCH            ${CMAKE_MATCH_3})
  set(OSRM_VERSION_PRERELEASE_BUILD ${CMAKE_MATCH_4})
  set(OSRM_VERSION "${packagejson.version}")
else()
  message(FATAL_ERROR "Version from package.json cannot be parsed. Expected one of:\n  - Monthly release format: (YYYY-2000).M.patchlevel with month 1-12, no leading zeros (e.g., 26.4.0)\n  - Legacy semver format: major.minor.patch with optional prerelease (e.g., 6.0.0 or 6.0.0-rc1)\nBut found: ${packagejson.version}")
endif()

if (MSVC)
  add_definitions("-DOSRM_PROJECT_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}\"")
else()
  add_definitions(-DOSRM_PROJECT_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
endif()

# these two functions build up custom variables:
#   DEPENDENCIES_INCLUDE_DIRS and OSRM_DEFINES
# These variables we want to pass to
# include_directories and add_definitions for both
# this build and for sharing externally via pkg-config

function(add_dependency_includes)
  if(${ARGC} GREATER 0)
    list(APPEND DEPENDENCIES_INCLUDE_DIRS "${ARGV}")
    set(DEPENDENCIES_INCLUDE_DIRS "${DEPENDENCIES_INCLUDE_DIRS}" PARENT_SCOPE)
  endif()
endfunction(add_dependency_includes)

function(add_dependency_defines defines)
  list(APPEND OSRM_DEFINES "${defines}")
  set(OSRM_DEFINES "${OSRM_DEFINES}" PARENT_SCOPE)
endfunction(add_dependency_defines)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
include(CheckCXXCompilerFlag)
include(FindPackageHandleStandardArgs)
include(GNUInstallDirs)

include_directories(BEFORE ${CMAKE_CURRENT_BINARY_DIR}/include/)
include_directories(BEFORE ${CMAKE_CURRENT_SOURCE_DIR}/include/)
include_directories(SYSTEM ${CMAKE_CURRENT_SOURCE_DIR}/generated/include/)

set(BOOST_COMPONENTS date_time iostreams program_options thread unit_test_framework)

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/include/util/version.hpp.in
  ${CMAKE_CURRENT_BINARY_DIR}/include/util/version.hpp
)
file(GLOB UtilGlob src/util/*.cpp src/util/*/*.cpp)
file(GLOB ExtractorGlob src/extractor/*.cpp src/extractor/*/*.cpp)
file(GLOB GuidanceGlob src/guidance/*.cpp src/extractor/intersection/*.cpp)
file(GLOB PartitionerGlob src/partitioner/*.cpp)
file(GLOB CustomizerGlob src/customize/*.cpp)
file(GLOB ContractorGlob src/contractor/*.cpp)
file(GLOB UpdaterGlob src/updater/*.cpp)
file(GLOB StorageGlob src/storage/*.cpp)
file(GLOB ServerGlob src/server/*.cpp src/server/**/*.cpp)
file(GLOB EngineGlob src/engine/*.cpp src/engine/**/*.cpp)

add_library(UTIL OBJECT ${UtilGlob})
# For historical reason the guidance and extract code are separate.
# They are so entangled though (circular dependencies) that separate libraries are not feasible.
add_library(EXTRACTOR OBJECT ${ExtractorGlob} ${GuidanceGlob})
add_library(PARTITIONER OBJECT ${PartitionerGlob})
add_library(CUSTOMIZER OBJECT ${CustomizerGlob})
add_library(CONTRACTOR OBJECT ${ContractorGlob})
add_library(UPDATER OBJECT ${UpdaterGlob})
add_library(STORAGE OBJECT ${StorageGlob})
add_library(ENGINE OBJECT ${EngineGlob})
add_library(SERVER OBJECT ${ServerGlob})

set_target_properties(UTIL PROPERTIES LINKER_LANGUAGE CXX)

# We will otherwise get false-positive errors in the linker stage (boost/qi) that can't be disabled with pragmas
set_property(TARGET SERVER PROPERTY INTERPROCEDURAL_OPTIMIZATION FALSE)

add_executable(osrm-routed src/tools/routed.cpp $<TARGET_OBJECTS:SERVER> $<TARGET_OBJECTS:UTIL>)
add_executable(osrm-extract src/tools/extract.cpp)
add_executable(osrm-partition src/tools/partition.cpp)
add_executable(osrm-customize src/tools/customize.cpp)
add_executable(osrm-contract src/tools/contract.cpp)
add_executable(osrm-datastore src/tools/store.cpp $<TARGET_OBJECTS:MICROTAR> $<TARGET_OBJECTS:UTIL>)
add_library(osrm src/osrm/osrm.cpp $<TARGET_OBJECTS:ENGINE> $<TARGET_OBJECTS:STORAGE> $<TARGET_OBJECTS:MICROTAR> $<TARGET_OBJECTS:UTIL>)
add_library(osrm_contract src/osrm/contractor.cpp $<TARGET_OBJECTS:CONTRACTOR> $<TARGET_OBJECTS:UTIL>)
add_library(osrm_extract src/osrm/extractor.cpp $<TARGET_OBJECTS:EXTRACTOR> $<TARGET_OBJECTS:MICROTAR> $<TARGET_OBJECTS:UTIL>)
add_library(osrm_partition src/osrm/partitioner.cpp $<TARGET_OBJECTS:PARTITIONER> $<TARGET_OBJECTS:MICROTAR> $<TARGET_OBJECTS:UTIL>)
add_library(osrm_customize src/osrm/customizer.cpp $<TARGET_OBJECTS:CUSTOMIZER> $<TARGET_OBJECTS:MICROTAR> $<TARGET_OBJECTS:UTIL>)
add_library(osrm_update $<TARGET_OBJECTS:UPDATER> $<TARGET_OBJECTS:MICROTAR> $<TARGET_OBJECTS:UTIL>)
add_library(osrm_store $<TARGET_OBJECTS:STORAGE> $<TARGET_OBJECTS:MICROTAR> $<TARGET_OBJECTS:UTIL>)

# ALIAS targets with :: provide compile-time safety: typos error at configure time, not link time
add_library(osrm::osrm ALIAS osrm)
add_library(osrm::contract ALIAS osrm_contract)
add_library(osrm::extract ALIAS osrm_extract)
add_library(osrm::partition ALIAS osrm_partition)
add_library(osrm::customize ALIAS osrm_customize)
add_library(osrm::update ALIAS osrm_update)
add_library(osrm::store ALIAS osrm_store)

# Explicitly set the build type to Release if no other type is specified
# on the command line.  Without this, cmake defaults to an unoptimized,
# non-debug build, which almost nobody wants.
if(NOT CMAKE_BUILD_TYPE)
  message(STATUS "No build type specified, defaulting to Release")
  set(CMAKE_BUILD_TYPE Release)
endif()

if(CMAKE_BUILD_TYPE MATCHES Debug)
  message(STATUS "Configuring OSRM in debug mode")
elseif(CMAKE_BUILD_TYPE MATCHES Release)
  message(STATUS "Configuring OSRM in release mode")
elseif(CMAKE_BUILD_TYPE MATCHES RelWithDebInfo)
  message(STATUS "Configuring OSRM in release mode with debug flags")
elseif(CMAKE_BUILD_TYPE MATCHES MinRelSize)
  message(STATUS "Configuring OSRM in release mode with minimized size")
else()
  message(STATUS "Unrecognized build type - will use cmake defaults")
endif()

# Additional logic for the different build types
if(CMAKE_BUILD_TYPE MATCHES Debug OR CMAKE_BUILD_TYPE MATCHES RelWithDebInfo)
  message(STATUS "Configuring debug mode flags")
  set(ENABLE_ASSERTIONS ON)
  set(ENABLE_DEBUG_LOGGING ON)
endif()

if(NOT CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
  set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} -fno-inline -fno-omit-frame-pointer")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-inline -fno-omit-frame-pointer")
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} -ggdb")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Og -ggdb")
endif()

set(MAYBE_COVERAGE_LIBRARIES "")
if (ENABLE_COVERAGE)
  if (NOT CMAKE_BUILD_TYPE MATCHES "Debug")
    message(ERROR "ENABLE_COVERAGE=ON only makes sense with a Debug build")
  endif()
  message(STATUS "Enabling coverage")
  set(MAYBE_COVERAGE_LIBRARIES "-lgcov")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -ftest-coverage -fprofile-arcs")
endif()


if (ENABLE_SANITIZER)
  set(SANITIZER_FLAGS "-g -fsanitize=address -fsanitize-address-use-after-scope -fsanitize=undefined -fno-omit-frame-pointer")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${SANITIZER_FLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${SANITIZER_FLAGS}")
  set(OSRM_CXXFLAGS "${OSRM_CXXFLAGS} ${SANITIZER_FLAGS}")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${SANITIZER_FLAGS}")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${SANITIZER_FLAGS}")
endif()

# Configuring compilers
include(cmake/warnings.cmake)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIC -fcolor-diagnostics -ftemplate-depth=1024")
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  set(COLOR_FLAG "-fdiagnostics-color=auto")
  check_cxx_compiler_flag("-fdiagnostics-color=auto" HAS_COLOR_FLAG)
  if(NOT HAS_COLOR_FLAG)
    set(COLOR_FLAG "")
  endif()
  # using GCC
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 ${COLOR_FLAG} -fPIC -ftemplate-depth=1024")

  if(WIN32) # using mingw
    add_dependency_defines(-DWIN32)
    set(OPTIONAL_SOCKET_LIBS ws2_32 wsock32)
  endif()
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Intel")
  # using Intel C++
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-intel -wd10237 -Wall -ipo -fPIC")
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
  # using Visual Studio C++
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj") # avoid compiler error C1128 from scripting_environment_lua.cpp
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /DWIN32_LEAN_AND_MEAN") # avoid compiler error C2011 from dual #include of winsock.h and winsock2.h
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8") # support Unicode in fmt library
  add_dependency_defines(-DBOOST_LIB_DIAGNOSTIC)
  add_dependency_defines(-D_CRT_SECURE_NO_WARNINGS)
  add_dependency_defines(-DNOMINMAX) # avoid min and max macros that can break compilation
  add_dependency_defines(-D_WIN32_WINNT=0x0501)
  add_dependency_defines(-DXML_STATIC)
  find_library(ws2_32_LIBRARY_PATH ws2_32)
  target_link_libraries(osrm-extract wsock32 ws2_32)
endif()

if(UNIX AND NOT APPLE)
  find_library(RT_LIB rt)
  if (RT_LIB)
    set(MAYBE_RT_LIBRARY -lrt)
  endif()
endif()

find_package(Threads REQUIRED)

# Check for C++20 <format> with full chrono support that compiles, links and runs
# This is necessary because some environments have the header but incomplete
# implementation (e.g., Alpine GCC 14 missing chrono formatters, or Clang
# with libstdc++ from older GCC lacking std::format symbols at link time)
include(CheckCXXSourceRuns)

if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
  set(CMAKE_REQUIRED_FLAGS "${CMAKE_CXX_FLAGS} /std:c++20")
else()
  set(CMAKE_REQUIRED_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20")
  set(CMAKE_REQUIRED_LIBRARIES "stdc++")
endif()

check_cxx_source_runs("
  #include <chrono>
  #include <format>
  #include <numbers>
  #include <string>
  int main() {
    // Test basic formatting
    std::string s1 = std::format(\"{:.10g}\", std::numbers::pi);
    // Test chrono formatting
    auto now = std::chrono::system_clock::now();
    auto secs = std::chrono::floor<std::chrono::seconds>(now);
    std::string s2 = std::format(\"{:%FT%T}\", secs);
    return (s1.empty() || s2.empty()) ? 1 : 0;
  }
" OSRM_HAS_STD_FORMAT)

unset(CMAKE_REQUIRED_FLAGS)
unset(CMAKE_REQUIRED_LIBRARIES)

if(OSRM_HAS_STD_FORMAT)
  add_definitions(-DOSRM_HAS_STD_FORMAT)
endif()

# microtar is still vendored: single .c file, no upstream vcpkg port.
set(MICROTAR_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/microtar/src")
include_directories(SYSTEM ${MICROTAR_INCLUDE_DIR})

add_library(MICROTAR OBJECT "${CMAKE_CURRENT_SOURCE_DIR}/third_party/microtar/src/microtar.c")
set_property(TARGET MICROTAR PROPERTY POSITION_INDEPENDENT_CODE ON)

target_no_warning(MICROTAR unused-variable)
target_no_warning(MICROTAR format)

# Header-only libraries from vcpkg. Each provides a CMake CONFIG package.
find_package(fmt CONFIG REQUIRED)
# fmt is consumed header-only so OSRM targets don't need to link libfmt.
# Without this, LTO builds emit undefined references to fmt::v12::vformat,
# fmt::v12::report_error, etc. because the templates pull in out-of-line
# symbols that only exist in the compiled fmt::fmt library.
add_compile_definitions(FMT_HEADER_ONLY)
find_package(RapidJSON CONFIG REQUIRED)
find_package(sol2 CONFIG REQUIRED)
find_package(flatbuffers CONFIG REQUIRED)

# protozero has no CMake config in its vcpkg port — it ships only headers.
# Locate the include dir via a sentinel header and add it directly.
find_path(PROTOZERO_INCLUDE_DIR NAMES protozero/pbf_reader.hpp REQUIRED)
include_directories(SYSTEM ${PROTOZERO_INCLUDE_DIR})

# vtzero has no vcpkg port; keep it vendored. Header-only, so just the include.
set(VTZERO_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/vtzero/include")
include_directories(SYSTEM ${VTZERO_INCLUDE_DIR})

# The OBJECT libraries below (UTIL, EXTRACTOR, ENGINE, …) were created before
# these find_package() calls, so we can't use target_link_libraries on them
# from here. Instead, harvest INTERFACE_INCLUDE_DIRECTORIES from each imported
# target and push them into the directory-level include_directories, which
# DOES apply retroactively to earlier targets. Consumers (unit tests, etc.)
# inherit the same search paths via the directory.
set(_osrm_vcpkg_header_targets
    fmt::fmt-header-only fmt::fmt
    sol2::sol2
    rapidjson RapidJSON::RapidJSON
    flatbuffers::flatbuffers)
foreach(_dep IN LISTS _osrm_vcpkg_header_targets)
  if(TARGET ${_dep})
    get_target_property(_inc ${_dep} INTERFACE_INCLUDE_DIRECTORIES)
    if(_inc)
      include_directories(SYSTEM ${_inc})
    endif()
  endif()
endforeach()


if (MSVC)
  # Disable Boost auto-linking on MSVC; we link boost libraries explicitly
  # via imported targets from vcpkg.
  add_definitions(-DBOOST_ALL_NO_LIB)
endif()


# All dependencies below come from vcpkg (vcpkg.json). The vcpkg toolchain
# file exposes them via standard CMake find_package() / imported targets.
find_package(Boost 1.83 REQUIRED CONFIG COMPONENTS ${BOOST_COMPONENTS})
find_package(TBB CONFIG REQUIRED)
# Always link the release build of TBB, even in Debug/Coverage configurations.
# vcpkg's debug TBB enables internal debug assertions (e.g. intrusive_list
# invariants) that trip during osrm-extract's test-data generation on
# ubuntu-24.04 under gcc-13. The master branch never hit this because it
# linked the system's release-only TBB. Do the same here.
foreach(_tbb_target TBB::tbb TBB::tbbmalloc TBB::tbbmalloc_proxy)
  if(TARGET ${_tbb_target})
    set_target_properties(${_tbb_target} PROPERTIES
      MAP_IMPORTED_CONFIG_DEBUG        Release
      MAP_IMPORTED_CONFIG_MINSIZEREL   Release
      MAP_IMPORTED_CONFIG_RELWITHDEBINFO Release)
  endif()
endforeach()
find_package(EXPAT REQUIRED)
find_package(BZip2 REQUIRED)
find_package(Lua 5.4 REQUIRED)

add_dependency_includes(${Boost_INCLUDE_DIRS})
add_dependency_includes(${EXPAT_INCLUDE_DIRS})
add_dependency_includes(${BZIP2_INCLUDE_DIR})
add_dependency_includes(${LUA_INCLUDE_DIR})

# Use modern imported targets for TBB and Boost so the rest of the build
# is agnostic to whether the deps came from vcpkg, a system package, or a
# subproject. Legacy Boost_*_LIBRARY variables below are populated to the
# matching target names so unit_tests/CMakeLists.txt and src/benchmarks/
# keep working without edits during the migration.
set(TBB_LIBRARIES TBB::tbb)

set(Boost_DATE_TIME_LIBRARY            Boost::date_time)
set(Boost_IOSTREAMS_LIBRARY            Boost::iostreams)
set(Boost_THREAD_LIBRARY               Boost::thread)
set(Boost_REGEX_LIBRARY                Boost::regex)
set(Boost_PROGRAM_OPTIONS_LIBRARY      Boost::program_options)
set(Boost_UNIT_TEST_FRAMEWORK_LIBRARY  Boost::unit_test_framework)

set(BOOST_BASE_LIBRARIES
  ${Boost_DATE_TIME_LIBRARY}
  ${Boost_IOSTREAMS_LIBRARY}
  ${Boost_THREAD_LIBRARY})

set(BOOST_ENGINE_LIBRARIES
  ${Boost_REGEX_LIBRARY}
  ${BOOST_BASE_LIBRARIES})

# add a target to generate API documentation with Doxygen
find_package(Doxygen)
if(DOXYGEN_FOUND)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
  if(BUILD_AS_SUBPROJECT)
    add_custom_target(osrm_doc
      ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      COMMENT "Generating API documentation with Doxygen" VERBATIM
    )
  else()
    add_custom_target(doc
      ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      COMMENT "Generating API documentation with Doxygen" VERBATIM
    )
  endif()
endif()

# libosmium from vcpkg. The vcpkg port ships only headers, so we use the
# upstream FindOsmium.cmake module (kept in cmake/) to locate the vcpkg
# include dir and wire up the expat/bzip2/zlib runtime libs via the "io"
# component.
find_package(Osmium REQUIRED COMPONENTS io)
include_directories(SYSTEM ${OSMIUM_INCLUDE_DIRS})

# prefix compilation with ccache by default if available and on clang or gcc
if(ENABLE_CCACHE AND (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU"))
  find_program(CCACHE_FOUND ccache)
  if(CCACHE_FOUND)
    message(STATUS "Using ccache to speed up incremental builds")
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
  endif()
endif()

# Link to zlib explicitly so osrm binaries play well with other binaries
# (e.g. nodejs) that expect the same zlib ABI.
find_package(ZLIB REQUIRED)
add_dependency_includes(${ZLIB_INCLUDE_DIRS})
set(ZLIB_LIBRARY ${ZLIB_LIBRARIES})

add_definitions(${OSRM_DEFINES})
include_directories(SYSTEM ${DEPENDENCIES_INCLUDE_DIRS})


# Binaries
target_link_libraries(osrm-datastore osrm_store ${Boost_PROGRAM_OPTIONS_LIBRARY})
target_link_libraries(osrm-extract osrm_extract ${Boost_PROGRAM_OPTIONS_LIBRARY})
target_link_libraries(osrm-partition osrm_partition ${Boost_PROGRAM_OPTIONS_LIBRARY})
target_link_libraries(osrm-customize osrm_customize ${Boost_PROGRAM_OPTIONS_LIBRARY})
target_link_libraries(osrm-contract osrm_contract ${Boost_PROGRAM_OPTIONS_LIBRARY})
target_link_libraries(osrm-routed osrm ${Boost_PROGRAM_OPTIONS_LIBRARY} ${OPTIONAL_SOCKET_LIBS} ${ZLIB_LIBRARY})

set(EXTRACTOR_LIBRARIES
    ${BZIP2_LIBRARIES}
    ${BOOST_BASE_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
    ${EXPAT_LIBRARIES}
    ${LUA_LIBRARIES}
    ${OSMIUM_LIBRARIES}
    ${TBB_LIBRARIES}
    ${ZLIB_LIBRARY}
    ${MAYBE_COVERAGE_LIBRARIES})
set(PARTITIONER_LIBRARIES
    ${BOOST_ENGINE_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
    ${TBB_LIBRARIES}
    ${MAYBE_RT_LIBRARY}
    ${MAYBE_COVERAGE_LIBRARIES})
set(CUSTOMIZER_LIBRARIES
    ${BOOST_ENGINE_LIBRARIES}
    ${ZLIB_LIBRARY}
    ${CMAKE_THREAD_LIBS_INIT}
    ${TBB_LIBRARIES}
    ${MAYBE_RT_LIBRARY}
    ${MAYBE_COVERAGE_LIBRARIES})
set(UPDATER_LIBRARIES
    ${BOOST_BASE_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
    ${TBB_LIBRARIES}
    ${MAYBE_RT_LIBRARY}
    ${MAYBE_COVERAGE_LIBRARIES}
    ${ZLIB_LIBRARY})
set(CONTRACTOR_LIBRARIES
    ${BOOST_BASE_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
    ${LUA_LIBRARIES}
    ${TBB_LIBRARIES}
    ${MAYBE_RT_LIBRARY}
    ${MAYBE_COVERAGE_LIBRARIES})
set(ENGINE_LIBRARIES
    ${BOOST_ENGINE_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
    ${TBB_LIBRARIES}
    ${MAYBE_RT_LIBRARY}
    ${MAYBE_COVERAGE_LIBRARIES}
    ${ZLIB_LIBRARY})
set(STORAGE_LIBRARIES
    ${BOOST_BASE_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
    ${TBB_LIBRARIES}
    ${MAYBE_RT_LIBRARY}
    ${MAYBE_COVERAGE_LIBRARIES})
set(UTIL_LIBRARIES
    ${BOOST_BASE_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
    ${TBB_LIBRARIES}
    ${MAYBE_COVERAGE_LIBRARIES}
    ${ZLIB_LIBRARY})

# Libraries
target_link_libraries(osrm ${ENGINE_LIBRARIES})
target_link_libraries(osrm_update ${UPDATER_LIBRARIES})
target_link_libraries(osrm_contract ${CONTRACTOR_LIBRARIES} osrm_update osrm_store)
target_link_libraries(osrm_extract ${EXTRACTOR_LIBRARIES})
target_link_libraries(osrm_partition ${PARTITIONER_LIBRARIES})
target_link_libraries(osrm_customize ${CUSTOMIZER_LIBRARIES} osrm_update osrm_store)
target_link_libraries(osrm_store ${STORAGE_LIBRARIES})

add_executable(osrm-components src/tools/components.cpp $<TARGET_OBJECTS:MICROTAR> $<TARGET_OBJECTS:UTIL>)
target_link_libraries(osrm-components ${TBB_LIBRARIES} ${BOOST_BASE_LIBRARIES} ${UTIL_LIBRARIES})
install(TARGETS osrm-components DESTINATION bin)

add_executable(osrm-io-benchmark src/tools/io-benchmark.cpp $<TARGET_OBJECTS:UTIL>)
target_link_libraries(osrm-io-benchmark ${BOOST_BASE_LIBRARIES} ${TBB_LIBRARIES})
install(TARGETS osrm-io-benchmark DESTINATION bin)

if (ENABLE_ASSERTIONS)
  message(STATUS "Enabling assertions")
  add_definitions(-DBOOST_ENABLE_ASSERT_HANDLER)
endif()

if (ENABLE_DEBUG_LOGGING)
  message(STATUS "Enabling debug logging")
  add_definitions(-DENABLE_DEBUG_LOGGING)
endif()

# Add RPATH info to executables so that when they are run after being installed
# (i.e., from /usr/local/bin/) the linker can find library dependencies. For
# more info see http://www.cmake.org/Wiki/CMake_RPATH_handling
set_property(TARGET osrm-extract PROPERTY INSTALL_RPATH_USE_LINK_PATH TRUE)
set_property(TARGET osrm-partition PROPERTY INSTALL_RPATH_USE_LINK_PATH TRUE)
set_property(TARGET osrm-contract PROPERTY INSTALL_RPATH_USE_LINK_PATH TRUE)
set_property(TARGET osrm-datastore PROPERTY INSTALL_RPATH_USE_LINK_PATH TRUE)
set_property(TARGET osrm-routed PROPERTY INSTALL_RPATH_USE_LINK_PATH TRUE)

file(GLOB LibraryGlob include/osrm/*.hpp)
file(GLOB ParametersGlob include/engine/api/*_parameters.hpp)
set(ApiHeader include/engine/api/base_result.hpp)
set(EngineHeader include/engine/status.hpp include/engine/engine_config.hpp include/engine/hint.hpp include/engine/bearing.hpp include/engine/approach.hpp include/engine/phantom_node.hpp)
set(UtilHeader include/util/coordinate.hpp include/util/json_container.hpp include/util/typedefs.hpp include/util/alias.hpp include/util/exception.hpp include/util/bearing.hpp)
set(ExtractorHeader include/extractor/extractor.hpp include/storage/io_config.hpp include/extractor/extractor_config.hpp include/extractor/travel_mode.hpp)
set(PartitionerHeader include/partitioner/partitioner.hpp include/partitioner/partitioner_config.hpp)
set(ContractorHeader include/contractor/contractor.hpp include/contractor/contractor_config.hpp)
set(StorageHeader include/storage/storage.hpp include/storage/io_config.hpp include/storage/storage_config.hpp)
install(FILES ${EngineHeader} DESTINATION include/osrm/engine)
install(FILES ${UtilHeader} DESTINATION include/osrm/util)
install(FILES ${StorageHeader} DESTINATION include/osrm/storage)
install(FILES ${ExtractorHeader} DESTINATION include/osrm/extractor)
install(FILES ${PartitionerHeader} DESTINATION include/osrm/partitioner)
install(FILES ${ContractorHeader} DESTINATION include/osrm/contractor)
install(FILES ${LibraryGlob} DESTINATION include/osrm)
install(FILES ${ParametersGlob} DESTINATION include/osrm/engine/api)
install(FILES ${ApiHeader} DESTINATION include/osrm/engine/api)
# Flatbuffers headers are provided by the vcpkg port and installed into the
# consumer's sysroot directly; we no longer ship them as part of libosrm.

install(TARGETS osrm-extract DESTINATION bin)
install(TARGETS osrm-partition DESTINATION bin)
install(TARGETS osrm-customize DESTINATION bin)
install(TARGETS osrm-contract DESTINATION bin)
install(TARGETS osrm-datastore DESTINATION bin)
install(TARGETS osrm-routed DESTINATION bin)

install(TARGETS osrm DESTINATION lib)
install(TARGETS osrm_extract DESTINATION lib)
install(TARGETS osrm_partition DESTINATION lib)
install(TARGETS osrm_customize DESTINATION lib)
install(TARGETS osrm_update DESTINATION lib)
install(TARGETS osrm_contract DESTINATION lib)
install(TARGETS osrm_store DESTINATION lib)

# Install profiles and support library to /usr/local/share/osrm/profiles by default
set(DefaultProfilesDir profiles)
install(DIRECTORY ${DefaultProfilesDir} DESTINATION share/osrm)

# Install data geojson files to /usr/local/share/osrm/data by default
set(DefaultProfilesDir data)
install(DIRECTORY ${DefaultProfilesDir} DESTINATION share/osrm)

if(BUILD_PACKAGE)
  include(CPackConfig)
  include(CPack)
endif()

function(JOIN VALUES GLUE OUTPUT)
  string (REPLACE ";" "${GLUE}" _TMP_STR "${VALUES}")
  set (${OUTPUT} "${_TMP_STR}" PARENT_SCOPE)
endfunction()

JOIN("${OSRM_DEFINES}" " " TMP_OSRM_DEFINES)
set(LibOSRM_CXXFLAGS "${OSRM_CXXFLAGS} ${TMP_OSRM_DEFINES}")
set(LibOSRM_LDFLAGS "${OSRM_LDFLAGS}")

if(BUILD_AS_SUBPROJECT)
  set(LibOSRM_CXXFLAGS "${LibOSRM_CXXFLAGS}" PARENT_SCOPE)
  set(LibOSRM_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include" PARENT_SCOPE)
  set(LibOSRM_LIBRARY_DIR "${CMAKE_CURRENT_BINARY_DIR}" PARENT_SCOPE)
  set(LibOSRM_LIBRARIES "osrm" PARENT_SCOPE)
  set(LibOSRM_DEPENDENT_LIBRARIES "${ENGINE_LIBRARIES}" PARENT_SCOPE)
  set(LibOSRM_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/include"
                           "${CMAKE_CURRENT_SOURCE_DIR}/include/osrm"
                           "${CMAKE_CURRENT_SOURCE_DIR}/third_party"
                           "${DEPENDENCIES_INCLUDE_DIRS}" PARENT_SCOPE)
  set(LibOSRM_LIBRARY_DIRS "${LibOSRM_LIBRARY_DIR}" PARENT_SCOPE)
endif()

# pkgconfig defines
set(PKGCONFIG_OSRM_CXXFLAGS "${LibOSRM_CXXFLAGS}")
set(PKGCONFIG_OSRM_LDFLAGS "${LibOSRM_LDFLAGS}")
set(PKGCONFIG_LIBRARY_DIR "${CMAKE_INSTALL_PREFIX}/lib")
set(PKGCONFIG_INCLUDE_DIR "${CMAKE_INSTALL_PREFIX}/include")

list(APPEND DEPENDENCIES_INCLUDE_DIRS "${PKGCONFIG_INCLUDE_DIR}")
list(APPEND DEPENDENCIES_INCLUDE_DIRS "${PKGCONFIG_INCLUDE_DIR}/osrm")
JOIN("-I${DEPENDENCIES_INCLUDE_DIRS}" " -I" PKGCONFIG_OSRM_INCLUDE_FLAGS)


foreach(engine_lib ${ENGINE_LIBRARIES})
  if(TARGET ${engine_lib})
    # Some imported targets (e.g. Boost::regex in modern Boost) are
    # INTERFACE-only and have no TARGET_LINKER_FILE. Only embed a linker
    # file for targets that actually produce one.
    get_target_property(_tgt_type ${engine_lib} TYPE)
    if(_tgt_type STREQUAL "INTERFACE_LIBRARY")
      list(APPEND PKGCONFIG_DEPENDENT_LIBRARIES "-l${engine_lib}")
    else()
      list(APPEND PKGCONFIG_DEPENDENT_LIBRARIES "$<TARGET_LINKER_FILE:${engine_lib}>")
    endif()
  else()
    list(APPEND PKGCONFIG_DEPENDENT_LIBRARIES "${engine_lib}")
  endif()
endforeach(engine_lib)

JOIN("${PKGCONFIG_DEPENDENT_LIBRARIES}" " " PKGCONFIG_OSRM_DEPENDENT_LIBRARIES)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/pkgconfig.in pkgconfig.configured @ONLY)
# Multi-config generators (e.g. Visual Studio) produce different values for
# $<TARGET_LINKER_FILE:...> per configuration, which causes file(GENERATE) to
# fail when writing a single output path. pkgconfig is not used on Windows, so
# skip generation for multi-config generators.
get_property(_is_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(NOT _is_multi_config)
  file(GENERATE
       OUTPUT
       ${PROJECT_BINARY_DIR}/libosrm.pc
       INPUT
       ${PROJECT_BINARY_DIR}/pkgconfig.configured)

  install(FILES ${PROJECT_BINARY_DIR}/libosrm.pc DESTINATION ${PKGCONFIG_LIBRARY_DIR}/pkgconfig)
endif()

# uninstall target
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

if(BUILD_AS_SUBPROJECT)
  add_custom_target(osrm_uninstall
      COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake/cmake_uninstall.cmake)
else()
  add_custom_target(uninstall
      COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake/cmake_uninstall.cmake)
endif()


# Modular build system: each directory registered here provides its own CMakeLists.txt
add_subdirectory(unit_tests)
add_subdirectory(src/benchmarks)

if (ENABLE_NODE_BINDINGS)
  add_subdirectory(src/nodejs)
endif()

if (ENABLE_FUZZING)
  # Requires libosrm being built with sanitizers; make configurable and default to ubsan
  set(FUZZ_SANITIZER "undefined" CACHE STRING "Sanitizer to be used for Fuzz testing")
  set_property(CACHE FUZZ_SANITIZER PROPERTY STRINGS "undefined" "integer" "address" "memory" "thread" "leak")

  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize-coverage=edge,indirect-calls,8bit-counters -fsanitize=address")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")
  set(OSRM_LDFLAGS "${OSRM_LDFLAGS} -fsanitize=address")

  message(STATUS "Using -fsanitize=${FUZZ_SANITIZER} for Fuzz testing")

  add_subdirectory(fuzz)
endif ()

# add headers sanity check target that includes all headers independently
set(check_headers_dir "${PROJECT_BINARY_DIR}/check-headers")
file(GLOB_RECURSE headers_to_check
  ${PROJECT_BINARY_DIR}/*.hpp
  ${PROJECT_SOURCE_DIR}/include/*.hpp)
foreach(header ${headers_to_check})
  if ("${header}" MATCHES ".*/include/nodejs/.*")
    # we do not check NodeJS bindings headers
    continue()
  endif()
  get_filename_component(filename ${header} NAME_WE)
  set(filename "${check_headers_dir}/${filename}.cpp")
  if (NOT EXISTS ${filename})
    file(WRITE ${filename} "#include \"${header}\"\n")
  endif()
  list(APPEND sources ${filename})
endforeach()
add_library(check-headers STATIC EXCLUDE_FROM_ALL ${sources})
set_target_properties(check-headers PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${check_headers_dir})
