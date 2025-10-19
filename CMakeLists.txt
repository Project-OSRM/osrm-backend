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
option(ENABLE_CONAN "Use conan for dependencies" OFF)
option(ENABLE_COVERAGE "Build with coverage instrumentalisation" OFF)
option(ENABLE_DEBUG_LOGGING "Use debug logging in release mode" OFF)
option(ENABLE_FUZZING "Fuzz testing using LLVM's libFuzzer" OFF)
option(ENABLE_LTO "Use Link Time Optimisation" ON)
option(ENABLE_NODE_BINDINGS "Build NodeJs bindings" OFF)
option(ENABLE_SANITIZER "Use memory sanitizer for Debug build" OFF)

if (ENABLE_CONAN)
  message(STATUS "Installing Conan packages. It may take a while...")
  find_program(CONAN_EXECUTABLE NAMES conan)

  if (NOT CONAN_EXECUTABLE)
    message(FATAL_ERROR "Conan not found! Please install Conan 2.x and try again.")
  else()
    set(CMAKE_TOOLCHAIN_FILE "./conan_toolchain.cmake")

    execute_process(
      COMMAND ${CONAN_EXECUTABLE} install .. --output-folder=. --build=missing --settings compiler.cppstd=20 --settings build_type=${CMAKE_BUILD_TYPE}
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      OUTPUT_VARIABLE conan_stdout
      ERROR_VARIABLE conan_stderr
      RESULT_VARIABLE conan_install_result
    )

    if (NOT conan_install_result EQUAL 0)
      message(FATAL_ERROR "Conan install failed: ${conan_install_result}. Stderr: ${conan_stderr}. Stdout: ${conan_stdout}")
    endif()
  endif()
endif()

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
else()
  set(CMAKE_BUILD_RPATH "\$ORIGIN")
  # https://stackoverflow.com/questions/6324131/rpath-origin-not-having-desired-effect
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-z,origin")
endif()

include(JSONParser)
file(READ "package.json" packagejsonraw)
sbeParseJson(packagejson packagejsonraw)

# This regex is not strict enough, but the correct one is too complicated for cmake matching.
# https://semver.org/#is-there-a-suggested-regular-expression-regex-to-check-a-semver-string
if (packagejson.version MATCHES "^([0-9]+)\.([0-9]+)\.([0-9]+)([-+][0-9a-zA-Z.-]+)?$")
  set(OSRM_VERSION_MAJOR            ${CMAKE_MATCH_1})
  set(OSRM_VERSION_MINOR            ${CMAKE_MATCH_2})
  set(OSRM_VERSION_PATCH            ${CMAKE_MATCH_3})
  set(OSRM_VERSION_PRERELEASE_BUILD ${CMAKE_MATCH_4})

  set(OSRM_VERSION packagejson.version)
else()
  message(FATAL_ERROR "Version from package.json cannot be parsed, expected semver compatible label, but found ${packagejson.version}")
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
include_directories(SYSTEM ${CMAKE_CURRENT_SOURCE_DIR}/third_party/sol2/include)

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
add_library(EXTRACTOR OBJECT ${ExtractorGlob})
add_library(GUIDANCE OBJECT ${GuidanceGlob})
add_library(PARTITIONER OBJECT ${PartitionerGlob})
add_library(CUSTOMIZER OBJECT ${CustomizerGlob})
add_library(CONTRACTOR OBJECT ${ContractorGlob})
add_library(UPDATER OBJECT ${UpdaterGlob})
add_library(STORAGE OBJECT ${StorageGlob})
add_library(ENGINE OBJECT ${EngineGlob})
add_library(SERVER OBJECT ${ServerGlob})

set_target_properties(UTIL PROPERTIES LINKER_LANGUAGE CXX)

add_executable(osrm-routed src/tools/routed.cpp $<TARGET_OBJECTS:SERVER> $<TARGET_OBJECTS:UTIL>)
add_executable(osrm-extract src/tools/extract.cpp)
add_executable(osrm-partition src/tools/partition.cpp)
add_executable(osrm-customize src/tools/customize.cpp)
add_executable(osrm-contract src/tools/contract.cpp)
add_executable(osrm-datastore src/tools/store.cpp $<TARGET_OBJECTS:MICROTAR> $<TARGET_OBJECTS:UTIL>)
add_library(osrm src/osrm/osrm.cpp $<TARGET_OBJECTS:ENGINE> $<TARGET_OBJECTS:STORAGE> $<TARGET_OBJECTS:MICROTAR> $<TARGET_OBJECTS:UTIL>)
add_library(osrm_contract src/osrm/contractor.cpp $<TARGET_OBJECTS:CONTRACTOR> $<TARGET_OBJECTS:UTIL>)
add_library(osrm_extract src/osrm/extractor.cpp $<TARGET_OBJECTS:EXTRACTOR> $<TARGET_OBJECTS:MICROTAR> $<TARGET_OBJECTS:UTIL>)
add_library(osrm_guidance $<TARGET_OBJECTS:GUIDANCE> $<TARGET_OBJECTS:UTIL>)
add_library(osrm_partition src/osrm/partitioner.cpp $<TARGET_OBJECTS:PARTITIONER> $<TARGET_OBJECTS:MICROTAR> $<TARGET_OBJECTS:UTIL>)
add_library(osrm_customize src/osrm/customizer.cpp $<TARGET_OBJECTS:CUSTOMIZER> $<TARGET_OBJECTS:MICROTAR> $<TARGET_OBJECTS:UTIL>)
add_library(osrm_update $<TARGET_OBJECTS:UPDATER> $<TARGET_OBJECTS:MICROTAR> $<TARGET_OBJECTS:UTIL>)
add_library(osrm_store $<TARGET_OBJECTS:STORAGE> $<TARGET_OBJECTS:MICROTAR> $<TARGET_OBJECTS:UTIL>)

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

# Third-party libraries
set(RAPIDJSON_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/rapidjson/include")
include_directories(SYSTEM ${RAPIDJSON_INCLUDE_DIR})

set(MICROTAR_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/microtar/src")
include_directories(SYSTEM ${MICROTAR_INCLUDE_DIR})

add_library(MICROTAR OBJECT "${CMAKE_CURRENT_SOURCE_DIR}/third_party/microtar/src/microtar.c")
set_property(TARGET MICROTAR PROPERTY POSITION_INDEPENDENT_CODE ON)

target_no_warning(MICROTAR unused-variable)
target_no_warning(MICROTAR format)

set(PROTOZERO_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/protozero/include")
include_directories(SYSTEM ${PROTOZERO_INCLUDE_DIR})

set(VTZERO_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/vtzero/include")
include_directories(SYSTEM ${VTZERO_INCLUDE_DIR})

set(FLATBUFFERS_BUILD_TESTS OFF CACHE BOOL "Disable the build of Flatbuffers tests and samples.")
set(FLATBUFFERS_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/flatbuffers")
set(FLATBUFFERS_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/flatbuffers/include")
include_directories(SYSTEM ${FLATBUFFERS_INCLUDE_DIR})
add_subdirectory(${FLATBUFFERS_SRC_DIR}
        ${CMAKE_CURRENT_BINARY_DIR}/flatbuffers-build
        EXCLUDE_FROM_ALL)

set(FMT_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/fmt/include")
add_compile_definitions(FMT_HEADER_ONLY)
include_directories(SYSTEM ${FMT_INCLUDE_DIR})


# see https://stackoverflow.com/questions/70898030/boost-link-error-using-conan-find-package
if (MSVC)
  add_definitions(-DBOOST_ALL_NO_LIB)
endif()


if (ENABLE_CONAN)
  set(Boost_USE_STATIC_LIBS ON)
  find_package(Boost REQUIRED COMPONENTS ${BOOST_COMPONENTS})
  find_package(TBB REQUIRED)
  find_package(EXPAT REQUIRED)
  find_package(BZip2 REQUIRED)
  find_package(Lua 5.2 REQUIRED)

  add_dependency_includes(${Boost_INCLUDE_DIRS})

  set(BOOST_BASE_LIBRARIES ${Boost_LIBRARIES})
  set(BOOST_ENGINE_LIBRARIES ${Boost_LIBRARIES})

  add_dependency_includes(${TBB_INCLUDE_DIR})
  set(TBB_LIBRARIES ${TBB_LIBRARIES})

  add_dependency_includes(${expat_INCLUDE_DIRS})
  set(EXPAT_LIBRARIES ${expat_LIBRARIES})
  set(EXPAT_INCLUDE_DIRS ${expat_INCLUDE_DIRS})

  add_dependency_includes(${BZIP2_INCLUDE_DIR})

  set(LUA_LIBRARIES lua::lua)
  if (LUA_FOUND)
    message(STATUS "Using Lua ${LUA_VERSION_STRING}")
  endif()

  add_dependency_includes(${lua_INCLUDE_DIRS})

  # note: we avoid calling find_package(Osmium ...) here to ensure that the
  # expat and bzip2 are used from conan rather than the system
  include_directories(SYSTEM ${CMAKE_CURRENT_SOURCE_DIR}/third_party/libosmium/include)
else()
  find_package(Boost REQUIRED CONFIG COMPONENTS ${BOOST_COMPONENTS})
  find_package(TBB REQUIRED)
  find_package(EXPAT REQUIRED)
  find_package(BZip2 REQUIRED)
  find_package(Lua 5.2 REQUIRED)

  add_dependency_includes(${Boost_INCLUDE_DIRS})
  add_dependency_includes(${TBB_INCLUDE_DIR})
  add_dependency_includes(${EXPAT_INCLUDE_DIRS})
  add_dependency_includes(${BZIP2_INCLUDE_DIR})
  add_dependency_includes(${LUA_INCLUDE_DIR})

  set(TBB_LIBRARIES TBB::tbb)

  set(BOOST_BASE_LIBRARIES
    ${Boost_DATE_TIME_LIBRARY}
    ${Boost_IOSTREAMS_LIBRARY}
    ${Boost_THREAD_LIBRARY})

  set(BOOST_ENGINE_LIBRARIES
    ${Boost_ZLIB_LIBRARY}
    ${Boost_REGEX_LIBRARY}
    ${BOOST_BASE_LIBRARIES})

  # add a target to generate API documentation with Doxygen
  find_package(Doxygen)
  if(DOXYGEN_FOUND)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
    add_custom_target(doc
      ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      COMMENT "Generating API documentation with Doxygen" VERBATIM
    )
  endif()
  # note libosmium depends on expat and bzip2
  list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/third_party/libosmium/cmake")
  if(NOT OSMIUM_INCLUDE_DIR)
    set(OSMIUM_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/libosmium/include")
  endif()
  find_package(Osmium REQUIRED COMPONENTS io)
  include_directories(SYSTEM ${OSMIUM_INCLUDE_DIR})
endif()



# prefix compilation with ccache by default if available and on clang or gcc
if(ENABLE_CCACHE AND (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU"))
  find_program(CCACHE_FOUND ccache)
  if(CCACHE_FOUND)
    message(STATUS "Using ccache to speed up incremental builds")
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
  endif()
endif()

# even with conan builds we want to link to system zlib
# to ensure that osrm binaries play well with other binaries like nodejs
find_package(ZLIB REQUIRED)
add_dependency_includes(${ZLIB_INCLUDE_DIRS})
set(ZLIB_LIBRARY ${ZLIB_LIBRARIES})

add_dependency_defines(-DBOOST_SPIRIT_USE_PHOENIX_V3)
add_dependency_defines(-DBOOST_RESULT_OF_USE_DECLTYPE)

# Workaround for https://github.com/boostorg/phoenix/issues/111
add_dependency_defines(-DBOOST_PHOENIX_STL_TUPLE_H_)

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
set(GUIDANCE_LIBRARIES
    ${BOOST_BASE_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
    ${LUA_LIBRARIES}
    ${TBB_LIBRARIES}
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
target_link_libraries(osrm_extract osrm_guidance ${EXTRACTOR_LIBRARIES})
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

file(GLOB FlatbuffersGlob third_party/flatbuffers/include/flatbuffers/*.h)
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
install(FILES ${FlatbuffersGlob} DESTINATION include/flatbuffers)

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
install(TARGETS osrm_guidance DESTINATION lib)

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


if (NOT ENABLE_CONAN)
  foreach(engine_lib ${ENGINE_LIBRARIES})
    if("${engine_lib}" MATCHES "^boost.*" OR "${engine_lib}" MATCHES "^TBB.*")
      list(APPEND PKGCONFIG_DEPENDENT_LIBRARIES "$<TARGET_LINKER_FILE:${engine_lib}>")
    else()
      list(APPEND PKGCONFIG_DEPENDENT_LIBRARIES "${engine_lib}")
    endif()
  endforeach(engine_lib)
endif()

JOIN("${PKGCONFIG_DEPENDENT_LIBRARIES}" " " PKGCONFIG_OSRM_DEPENDENT_LIBRARIES)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/pkgconfig.in pkgconfig.configured @ONLY)
file(GENERATE
     OUTPUT
     ${PROJECT_BINARY_DIR}/libosrm.pc
     INPUT
     ${PROJECT_BINARY_DIR}/pkgconfig.configured)

install(FILES ${PROJECT_BINARY_DIR}/libosrm.pc DESTINATION ${PKGCONFIG_LIBRARY_DIR}/pkgconfig)

# uninstall target
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake/cmake_uninstall.cmake)


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
