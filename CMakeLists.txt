cmake_minimum_required(VERSION 3.31)
message(STATUS "CMake version: ${CMAKE_VERSION}")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
# set(CMAKE_VERBOSE_MAKEFILE ON)

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

option(BUILD_PACKAGE        "Build the OSRM package")
option(BUILD_NODE_PACKAGE   "Build the NodeJS package")
option(BUILD_SHARED_LIBS    "Build with shared libs")
option(ENABLE_ASSERTIONS    "Use assertions in release mode")
option(ENABLE_CCACHE        "Speed up incremental rebuilds via ccache (default ON)" ON)
option(ENABLE_COVERAGE      "Build with coverage instrumentalisation")
option(ENABLE_DEBUG_LOGGING "Use debug logging in release mode")
option(ENABLE_FUZZING       "Fuzz testing using LLVM's libFuzzer")
option(ENABLE_LTO           "Use Link Time Optimisation (default ON)" ON)
option(ENABLE_ASAN          "Use address sanitizer for Debug build")
option(ENABLE_TSAN          "Use thread sanitizer for Debug build (experimental)")
option(ENABLE_UBSAN         "Use undefined behaviour sanitizer for Debug build")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

project(OSRM C CXX)

# use ccache/sccache if available
if(ENABLE_CCACHE)
  set(USE_CCACHE "ccache")
endif()
if(ENABLE_SCCACHE)
  set(USE_CCACHE "sccache")
endif()
if(USE_CCACHE)
  find_program(CCACHE_BINARY ${USE_CCACHE})
endif()
if(CCACHE_BINARY)
  set(CMAKE_C_COMPILER_LAUNCHER ${CCACHE_BINARY})
  set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_BINARY})
  if(MSVC)
    # By default Visual Studio generators will use /Zi which is not compatible
    # with ccache, so tell Visual Studio to use /Z7 instead.
    set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<$<CONFIG:Debug,RelWithDebInfo>:Embedded>")
    cmake_policy(SET CMP0141 NEW) # re. MSVC debug information
    # Note: The CMAKE_<LANG>_COMPILER_LAUNCHER trick does not work with the Visual Studio generator!
    # This is a workaround:
    file(COPY_FILE "${CCACHE_BINARY}" "${CMAKE_BINARY_DIR}/cl.exe" ONLY_IF_DIFFERENT)
    set(CMAKE_VS_GLOBALS
      "CLToolExe=cl.exe"
      "CLToolPath=${CMAKE_BINARY_DIR}"
      "UseMultiToolTask=true"
      "TrackFileAccess=false"
    )
    # sccache (v0.13.0) thinks the /scanDependencies argument is "multiple input files"
    set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
  endif()
endif()

if(CLANG_CXX_CLANG_TIDY)
  find_program(CLANG_CXX_CLANG_TIDY ${CLANG_CXX_CLANG_TIDY})
endif()

set(CMAKE_INTERPROCEDURAL_OPTIMIZATION FALSE)
if(ENABLE_LTO AND (CMAKE_BUILD_TYPE MATCHES Release OR CMAKE_BUILD_TYPE MATCHES MinRelSize OR CMAKE_BUILD_TYPE MATCHES RelWithDebInfo))
  include(CheckIPOSupported)
  check_ipo_supported(RESULT LTO_SUPPORTED OUTPUT error)
  if(LTO_SUPPORTED)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
  else()
    message(FATAL_ERROR "IPO / LTO not supported: <${error}>")
  endif()
endif()

# add @loader_path/$ORIGIN to rpath to make binaries relocatable
if(APPLE)
  set(CMAKE_BUILD_RPATH "@loader_path")
else()
  set(CMAKE_BUILD_RPATH "\$ORIGIN")
  if(NOT MSVC)
    # https://stackoverflow.com/questions/6324131/rpath-origin-not-having-desired-effect
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-z,origin")
    set(CMAKE_EXE_LINKER_FLAGS       "${CMAKE_EXE_LINKER_FLAGS} -Wl,-z,origin")

    # Fix for `cmake --install --component node_osrm` rising the error: "file Could not
    # resolve runtime dependencies: libboost_system.so.1.88.0".  RUNPATH and RPATH are
    # different animals, see: `man ld.so`.  While the RPATH of the grandparent is used
    # when searching for dependencies the RUNPATH of the grandparent is not.  Thus we
    # have to disable DT_RUNPATH and use DT_RPATH instead because we have the
    # dependency: node_osrm.node => libboost_iostreams.so => libboost_system.so and
    # there is no RUNPATH set in libboost_iostreams.so.
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--disable-new-dtags")
    set(CMAKE_EXE_LINKER_FLAGS       "${CMAKE_EXE_LINKER_FLAGS} -Wl,--disable-new-dtags")
  endif()
endif()

include(JSONParser)
file(READ "package.json" packagejsonraw)
sbeParseJson(packagejson packagejsonraw)

# This regex is not strict enough, but the correct one is too complicated for cmake matching.
# https://semver.org/#is-there-a-suggested-regular-expression-regex-to-check-a-semver-string
if(packagejson.version MATCHES "^([0-9]+)\.([0-9]+)\.([0-9]+)([-+][0-9a-zA-Z.-]+)?$")
  set(OSRM_VERSION_MAJOR            ${CMAKE_MATCH_1})
  set(OSRM_VERSION_MINOR            ${CMAKE_MATCH_2})
  set(OSRM_VERSION_PATCH            ${CMAKE_MATCH_3})
  set(OSRM_VERSION_PRERELEASE_BUILD ${CMAKE_MATCH_4})

  set(OSRM_VERSION packagejson.version)
else()
  message(FATAL_ERROR "Version from package.json cannot be parsed, expected semver compatible label, but found ${packagejson.version}")
endif()

if(MSVC)
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

# Following lines and functions generate the files:
# - `build/osrm-run-env.sh` usage: `source build/osrm-run-env.sh`
# - `build/osrm-run.env`    usage: `cat build/osrm-run.env >> $GIHUB_ENV`.

set(OSRM_RUN_ENV    "${PROJECT_SOURCE_DIR}/build/osrm-run.env")
set(OSRM_RUN_ENV_SH "${PROJECT_SOURCE_DIR}/build/osrm-run-env.sh")
file(REMOVE "${OSRM_RUN_ENV}")
configure_file(cmake/osrm-run-env.sh.in ${OSRM_RUN_ENV_SH} @ONLY)

function(push_env key value)
  # puts key=value into osrm-run.env
  file(APPEND "${OSRM_RUN_ENV}" "${key}=${value}\n")
  # ENV used as workaround for setting variables on the project scope
  set(ENV{${key}} "${value}")
  set(${key} "${value}" PARENT_SCOPE)
endfunction()

# this functions appends /Release and /Debug for multi-config generators
# usage: mk_build_dir(OSRM_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}")
function(mk_build_dir key value)
  if(CMAKE_CONFIGURATION_TYPES)
    # multi-config generators (eg. Visual Studio) append /Release or /Debug
    cmake_path(APPEND dir "${value}" "$ENV{OSRM_CONFIG}")
  else()
    # single-config generator (eg. Unix Makefiles)
    cmake_path(SET dir "${value}")
  endif()
  set(${key} "${dir}" PARENT_SCOPE)
endfunction()

mk_build_dir("OSRM_BUILD_DIR" "${CMAKE_CURRENT_BINARY_DIR}")
push_env("OSRM_BUILD_DIR" "${OSRM_BUILD_DIR}")
push_env("OSRM_CONFIG" "${CMAKE_BUILD_TYPE}")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(NOT MSVC)
  add_custom_target(clangd-conf ALL
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      "${OSRM_BUILD_DIR}/compile_commands.json"
      "${PROJECT_SOURCE_DIR}/build/compile_commands.json")
endif()

include(CheckCXXCompilerFlag)
include(FindPackageHandleStandardArgs)
include(GNUInstallDirs)

include_directories(BEFORE ${CMAKE_CURRENT_BINARY_DIR}/include/)
include_directories(BEFORE ${CMAKE_CURRENT_SOURCE_DIR}/include/)
include_directories(SYSTEM ${CMAKE_CURRENT_SOURCE_DIR}/generated/include/)
include_directories(SYSTEM ${CMAKE_CURRENT_SOURCE_DIR}/third_party/sol2/include)

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/include/util/version.hpp.in
  ${CMAKE_CURRENT_BINARY_DIR}/include/util/version.hpp
)
file(GLOB ContractorGlob  src/contractor/*.cpp src/osrm/contractor.cpp)
file(GLOB CustomizerGlob  src/customize/*.cpp  src/osrm/customizer.cpp)
file(GLOB EngineGlob      src/engine/*.cpp src/engine/**/*.cpp src/osrm/osrm.cpp)
file(GLOB ExtractorGlob   src/extractor/*.cpp src/extractor/*/*.cpp src/osrm/extractor.cpp)
file(GLOB GuidanceGlob    src/guidance/*.cpp src/extractor/intersection/*.cpp)
file(GLOB PartitionerGlob src/partitioner/*.cpp src/osrm/partitioner.cpp)
file(GLOB ServerGlob      src/server/*.cpp src/server/**/*.cpp)
file(GLOB StorageGlob     src/storage/*.cpp)
file(GLOB UpdaterGlob     src/updater/*.cpp)
file(GLOB UtilGlob        src/util/*.cpp src/util/*/*.cpp)

# OBJECT libraries are just collections of *o files
# For historical reason the guidance and extract code are separate.
# They are so entangled though (circular dependencies) that separate libraries are not feasible.
add_library(CONTRACTOR  OBJECT ${ContractorGlob})
add_library(CUSTOMIZER  OBJECT ${CustomizerGlob})
add_library(ENGINE      OBJECT ${EngineGlob})
add_library(EXTRACTOR   OBJECT ${ExtractorGlob} ${GuidanceGlob})
add_library(PARTITIONER OBJECT ${PartitionerGlob})
add_library(SERVER      OBJECT ${ServerGlob})
add_library(STORAGE     OBJECT ${StorageGlob})
add_library(UPDATER     OBJECT ${UpdaterGlob})
add_library(UTIL        OBJECT ${UtilGlob})

set_target_properties(UTIL PROPERTIES LINKER_LANGUAGE CXX)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  # We will otherwise get false-positive errors in the linker stage (boost/qi) that can't be disabled with pragmas
  set_target_properties(SERVER PROPERTIES INTERPROCEDURAL_OPTIMIZATION FALSE)
endif()

if(NOT CMAKE_CONFIGURATION_TYPES)
  # single-config build generators like Unix Makefiles need a build type specified
  # multi-config generators take the cmake --config parameter later
  if(NOT CMAKE_BUILD_TYPE)
    # set(CACHE{CMAKE_BUILD_TYPE} VALUE "Release")
    set(CMAKE_BUILD_TYPE "Release")
  endif()

  if(CMAKE_BUILD_TYPE MATCHES Release)
    message(STATUS "Configuring OSRM in release mode")
  elseif(CMAKE_BUILD_TYPE MATCHES Debug)
    message(STATUS "Configuring OSRM in debug mode")
  elseif(CMAKE_BUILD_TYPE MATCHES RelWithDebInfo)
    message(STATUS "Configuring OSRM in release mode with debug flags")
  elseif(CMAKE_BUILD_TYPE MATCHES MinRelSize)
    message(STATUS "Configuring OSRM in release mode with minimized size")
  else()
    message(FATAL_ERROR "Unrecognized CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
Please use `Release', `Debug', `RelWithDebInfo', or `MinRelSize'.")
  endif()
endif()

# Additional logic for the different build types
if(CMAKE_BUILD_TYPE MATCHES Debug OR CMAKE_BUILD_TYPE MATCHES RelWithDebInfo)
  message(STATUS "Configuring debug mode flags")
  set(ENABLE_ASSERTIONS ON)
  set(ENABLE_DEBUG_LOGGING ON)
endif()

if(ENABLE_ASSERTIONS)
  message(STATUS "Enabling assertions")
  add_definitions(-DBOOST_ENABLE_ASSERT_HANDLER)
endif()

if(ENABLE_DEBUG_LOGGING)
  message(STATUS "Enabling debug logging")
  add_definitions(-DENABLE_DEBUG_LOGGING)
endif()

if(NOT MSVC)
  set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} -fno-inline -fno-omit-frame-pointer")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-inline -fno-omit-frame-pointer")
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} -ggdb")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Og -ggdb")
endif()

set(MAYBE_COVERAGE_LIBRARIES "")
if(ENABLE_COVERAGE)
  message(STATUS "Enabling coverage")
  set(MAYBE_COVERAGE_LIBRARIES "-lgcov")
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -ftest-coverage -fprofile-arcs")
  else()
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fprofile-instr-generate -fcoverage-mapping")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fcoverage-mapping")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fcoverage-mapping")
  endif()
endif()

if(ENABLE_ASAN)
  set(ASAN_FLAGS "-shared-libasan -fsanitize=address -fsanitize-address-use-after-scope")
endif()

if(ENABLE_TSAN)
  set(TSAN_FLAGS "-fsanitize=thread")
endif()

if(ENABLE_UBSAN)
  set(UBSAN_FLAGS "-fsanitize=undefined")
endif()

if(ENABLE_ASAN OR ENABLE_TSAN OR ENABLE_UBSAN)
  set(SANITIZER_FLAGS "-g ${ASAN_FLAGS} ${TSAN_FLAGS} ${UBSAN_FLAGS} -fno-omit-frame-pointer")

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
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Intel")
  # using Intel C++
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-intel -wd10237 -Wall -ipo -fPIC")
elseif(MSVC)
  # using Visual Studio C++
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj") # avoid compiler error C1128 from scripting_environment_lua.cpp
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /DWIN32_LEAN_AND_MEAN") # avoid compiler error C2011 from dual #include of winsock.h and winsock2.h
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8") # support Unicode in fmt library

  # see https://stackoverflow.com/questions/70898030/boost-link-error-using-conan-find-package
  # LINK : fatal error LNK1104: cannot open file 'libboost_iostreams-vc143-mt-x64-1_88.lib' [D:\a\osrm-backend\osrm-backend\build\osrm-datastore.vcxproj]
  # BOOST_ALL_NO_LIB: Tells the config system not to automatically select
  # which libraries to link against.
  # Normally if a compiler supports #pragma lib, then the correct library
  # build variant will be automatically selected and linked against,
  # simply by the act of including one of that library's headers.
  # This macro turns that feature off.

  # mmap_memory_allocator.obj : error LNK2005: "public: __cdecl boost::iostreams::mapped_file_source::~mapped_file_source(void)" (??1mapped_file_source@iostreams@boost@@QEAA@XZ) \
  #   already defined in boost_iostreams.lib(boost_iostreams.dll) [D:\a\osrm-backend\osrm-backend\build\osrm.vcxproj]
  # D:\a\osrm-backend\osrm-backend\build\Release\osrm.dll : fatal error LNK1169: one or more multiply defined symbols found [D:\a\osrm-backend\osrm-backend\build\osrm.vcxproj]
  # BOOST_ALL_DYN_LINK: Forces all libraries that have separate source,
  # to be linked as dll's rather than static libraries on Microsoft Windows
  # (this macro is used to turn on __declspec(dllimport) modifiers, so that
  # the compiler knows which symbols to look for in a dll rather than in a
  # static library).  Note that there may be some libraries that can only
  # be linked in one way (statically or dynamically), in these cases this
  # macro has no effect.

  if(USE_CONAN)
    add_dependency_defines(-DBOOST_ALL_NO_LIB)
    add_dependency_defines(-DBOOST_ALL_DYN_LINK)
  endif()

  add_dependency_defines(-D_CRT_SECURE_NO_WARNINGS)
  add_dependency_defines(-DNOMINMAX) # avoid min and max macros that can break compilation
  add_dependency_defines(-D_WIN32_WINNT=0x0A00) # Windows 10
  add_dependency_defines(-DXML_STATIC)
endif()

if(UNIX AND NOT APPLE)
  find_library(RT_LIB rt)
  if(RT_LIB)
    set(MAYBE_RT_LIBRARY -lrt)
  endif()
endif()

if (MINGW)
  find_library(ws2_32_LIBRARY_PATH ws2_32)
  # target_link_libraries(osrm-extract wsock32 ws2_32)
  add_dependency_defines(-DWIN32)
  set(MAYBE_SOCKET_LIBRARIES ws2_32 wsock32)
endif()

# Check for C++20 <format> with full chrono support that compiles, links and runs
# This is necessary because some environments have the header but incomplete
# implementation (e.g., Alpine GCC 14 missing chrono formatters, or Clang
# with libstdc++ from older GCC lacking std::format symbols at link time)
include(CheckCXXSourceRuns)

if(MSVC)
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

#
# Third-party libraries
#

message(STATUS "CMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}")

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

set(BOOST_COMPONENTS date_time iostreams program_options thread unit_test_framework)

set(Boost_VERBOSE ON)
find_package(Boost 1.70 REQUIRED COMPONENTS ${BOOST_COMPONENTS})
find_package(BZip2 REQUIRED)
find_package(EXPAT REQUIRED)
find_package(Lua 5.2 REQUIRED)
find_package(TBB REQUIRED)

if(USE_CONAN)
  # Note: prefer zlib from system, the same as nodejs will use
  find_path(ZLIB_H "zlib.h" PATHS "/usr/include" NO_DEFAULT_PATH)
endif()
if(NOT ZLIB_H)
  find_package(ZLIB REQUIRED)
  add_dependency_includes(${ZLIB_INCLUDE_DIRS})
endif()

# Note: prefer expat and bzip2 from Conan
# Osmium depends on expat and bzip2
add_dependency_includes(${Boost_INCLUDE_DIRS})
add_dependency_includes(${TBB_INCLUDE_DIRS})
add_dependency_includes(${BZIP2_INCLUDE_DIR})
add_dependency_includes(${EXPAT_INCLUDE_DIRS})
add_dependency_includes(${LUA_INCLUDE_DIR})

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/third_party/libosmium/cmake")
if(NOT OSMIUM_INCLUDE_DIR)
  # FindOsmium is too dumb to look for Osmium in its very own parent dir ...
  set(OSMIUM_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/third_party/libosmium/include")
endif()
find_package(Osmium REQUIRED COMPONENTS io)
add_dependency_includes(${OSMIUM_INCLUDE_DIR})

set(BOOST_BASE_LIBRARIES            Boost::date_time Boost::iostreams Boost::thread)
set(BOOST_ENGINE_LIBRARIES          ${BOOST_BASE_LIBRARIES})
set(BOOST_UNIT_TEST_LIBRARIES       Boost::unit_test_framework)
set(BOOST_PROGRAM_OPTIONS_LIBRARIES Boost::program_options)

# add a target to generate API documentation with Doxygen
find_package(Doxygen)
if(DOXYGEN_FOUND)
  set(OSRM_DOCS_BUILD_DIR ${PROJECT_SOURCE_DIR}/build/docs/)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in ${OSRM_DOCS_BUILD_DIR}/Doxyfile @ONLY)
  add_custom_target(docs
    ${DOXYGEN_EXECUTABLE} ${OSRM_DOCS_BUILD_DIR}/Doxyfile
    WORKING_DIRECTORY ${OSRM_DOCS_BUILD_DIR}
    COMMENT "Generating API documentation with Doxygen" VERBATIM
  )
endif()

# add_dependency_defines(-DBOOST_SPIRIT_USE_PHOENIX_V3)
# add_dependency_defines(-DBOOST_RESULT_OF_USE_DECLTYPE)

# Workaround for https://github.com/boostorg/phoenix/issues/111
# add_dependency_defines(-DBOOST_PHOENIX_STL_TUPLE_H_)

add_definitions(${OSRM_DEFINES})
list(REMOVE_DUPLICATES DEPENDENCIES_INCLUDE_DIRS)
include_directories(SYSTEM ${DEPENDENCIES_INCLUDE_DIRS})

set(UTIL_LIBRARIES
    ${BOOST_BASE_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
    TBB::tbb
    ${MAYBE_RT_LIBRARY}
    ${MAYBE_COVERAGE_LIBRARIES}
    ${ZLIB_LIBRARIES})

set(EXTRACTOR_LIBRARIES
    ${BZIP2_LIBRARIES}
    ${EXPAT_LIBRARIES}
    ${LUA_LIBRARIES}
    ${OSMIUM_LIBRARIES})

set(ENGINE_LIBRARIES
    ${MAYBE_SOCKET_LIBRARIES})

# Libraries
add_library(osrm_utils)   # used by osrm tools
add_library(osrm_routed)  # used by osrm-routed, node and subprojects

set_target_properties(osrm_routed PROPERTIES OUTPUT_NAME "osrm")

target_link_libraries(osrm_utils  UTIL STORAGE UPDATER MICROTAR ${UTIL_LIBRARIES})
target_link_libraries(osrm_routed ENGINE SERVER osrm_utils ${ENGINE_LIBRARIES})

# Binaries
add_executable(osrm-components   src/tools/components.cpp)
add_executable(osrm-contract     src/tools/contract.cpp)
add_executable(osrm-customize    src/tools/customize.cpp)
add_executable(osrm-datastore    src/tools/store.cpp)
add_executable(osrm-extract      src/tools/extract.cpp)
add_executable(osrm-io-benchmark src/tools/io-benchmark.cpp)
add_executable(osrm-partition    src/tools/partition.cpp)
add_executable(osrm-routed       src/tools/routed.cpp)

target_link_libraries(osrm-components   osrm_utils)
target_link_libraries(osrm-contract     osrm_utils CONTRACTOR    ${BOOST_PROGRAM_OPTIONS_LIBRARIES})
target_link_libraries(osrm-customize    osrm_utils CUSTOMIZER    ${BOOST_PROGRAM_OPTIONS_LIBRARIES})
target_link_libraries(osrm-datastore    osrm_utils               ${BOOST_PROGRAM_OPTIONS_LIBRARIES})
target_link_libraries(osrm-extract      osrm_utils EXTRACTOR     ${BOOST_PROGRAM_OPTIONS_LIBRARIES} ${EXTRACTOR_LIBRARIES})
target_link_libraries(osrm-io-benchmark osrm_utils)
target_link_libraries(osrm-partition    osrm_utils PARTITIONER   ${BOOST_PROGRAM_OPTIONS_LIBRARIES})
target_link_libraries(osrm-routed       osrm_routed              ${BOOST_PROGRAM_OPTIONS_LIBRARIES})

# to build a shared osrm_utils lib on Windows
#
# - insert #include "osrm_utils_export.hpp" into all UTIL headers
# - put 'OSRM_UTILS_EXPORT' in front of *every* exported function
#   in the UTIL source. (left as exercise)
# - uncomment the following lines:
#
# include(GenerateExportHeader)
# include_directories(BEFORE ${CMAKE_CURRENT_BINARY_DIR}/include/)
# generate_export_header(osrm_utils EXPORT_FILE_NAME include/osrm_utils_export.hpp)
# target_compile_definitions(UTIL PRIVATE osrm_utils_EXPORTS)
# end build shared libs on Windows

# Add RPATH info to executables so that when they are run after being installed
# (i.e., from /usr/local/bin/) the linker can find library dependencies. For
# more info see https://gitlab.kitware.com/cmake/community/-/wikis/doc/cmake/RPATH-handling
set_property(TARGET osrm-components   PROPERTY INSTALL_RPATH_USE_LINK_PATH TRUE)
set_property(TARGET osrm-contract     PROPERTY INSTALL_RPATH_USE_LINK_PATH TRUE)
set_property(TARGET osrm-customize    PROPERTY INSTALL_RPATH_USE_LINK_PATH TRUE)
set_property(TARGET osrm-datastore    PROPERTY INSTALL_RPATH_USE_LINK_PATH TRUE)
set_property(TARGET osrm-extract      PROPERTY INSTALL_RPATH_USE_LINK_PATH TRUE)
set_property(TARGET osrm-io-benchmark PROPERTY INSTALL_RPATH_USE_LINK_PATH TRUE)
set_property(TARGET osrm-partition    PROPERTY INSTALL_RPATH_USE_LINK_PATH TRUE)
set_property(TARGET osrm-routed       PROPERTY INSTALL_RPATH_USE_LINK_PATH TRUE)

### install ###

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

install(FILES ${EngineHeader}      DESTINATION include/osrm/engine      COMPONENT headers)
install(FILES ${UtilHeader}        DESTINATION include/osrm/util        COMPONENT headers)
install(FILES ${StorageHeader}     DESTINATION include/osrm/storage     COMPONENT headers)
install(FILES ${ExtractorHeader}   DESTINATION include/osrm/extractor   COMPONENT headers)
install(FILES ${PartitionerHeader} DESTINATION include/osrm/partitioner COMPONENT headers)
install(FILES ${ContractorHeader}  DESTINATION include/osrm/contractor  COMPONENT headers)
install(FILES ${LibraryGlob}       DESTINATION include/osrm             COMPONENT headers)
install(FILES ${ParametersGlob}    DESTINATION include/osrm/engine/api  COMPONENT headers)
install(FILES ${ApiHeader}         DESTINATION include/osrm/engine/api  COMPONENT headers)
install(FILES ${FlatbuffersGlob}   DESTINATION include/flatbuffers      COMPONENT flatbuffers)

install(TARGETS osrm-components   DESTINATION bin COMPONENT applications)
install(TARGETS osrm-contract     DESTINATION bin COMPONENT applications)
install(TARGETS osrm-customize    DESTINATION bin COMPONENT applications)
install(TARGETS osrm-datastore    DESTINATION bin COMPONENT applications)
install(TARGETS osrm-extract      DESTINATION bin COMPONENT applications)
install(TARGETS osrm-io-benchmark DESTINATION bin COMPONENT applications)
install(TARGETS osrm-partition    DESTINATION bin COMPONENT applications)
install(TARGETS osrm-routed       DESTINATION bin COMPONENT applications)

install(TARGETS osrm_utils  DESTINATION lib COMPONENT libraries)
install(TARGETS osrm_routed DESTINATION lib COMPONENT libraries)

# Install profiles and support library to /usr/local/share/osrm/profiles by default
install(DIRECTORY profiles DESTINATION share/osrm COMPONENT applications)

# Install data geojson files to /usr/local/share/osrm/data by default
install(DIRECTORY data DESTINATION share/osrm COMPONENT applications)

if(BUILD_PACKAGE)
  include(CPackConfig)
  include(CPack)

  cpack_add_component_group(Runtime     DISPLAY_NAME "Runtime")
  cpack_add_component_group(Development DISPLAY_NAME "Development")

  cpack_add_component(applications GROUP Runtime     DISPLAY_NAME "OSRM Application")
  cpack_add_component(libraries    GROUP Development DISPLAY_NAME "OSRM Libraries")
  cpack_add_component(headers      GROUP Development DISPLAY_NAME "OSRM Headers"      DEPENDS libraries)
  cpack_add_component(flatbuffers  GROUP Development DISPLAY_NAME "3rd-Party Headers" DEPENDS libraries DISABLED)
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
  set(LibOSRM_LIBRARIES "osrm_routed" PARENT_SCOPE)
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


if(NOT USE_CONAN)
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
if(BUILD_NODE_PACKAGE)
  add_subdirectory(src/nodejs nodejs)
endif()
enable_testing()  # enable here so tests will be visible from root
add_subdirectory(unit_tests)
add_subdirectory(src/benchmarks)

if(ENABLE_FUZZING)
  # Requires libosrm being built with sanitizers; make configurable and default to ubsan
  set(FUZZ_SANITIZER "undefined" CACHE STRING "Sanitizer to be used for Fuzz testing")
  set_property(CACHE FUZZ_SANITIZER PROPERTY STRINGS "undefined" "integer" "address" "memory" "thread" "leak")

  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize-coverage=edge,indirect-calls,8bit-counters -fsanitize=address")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")
  set(OSRM_LDFLAGS "${OSRM_LDFLAGS} -fsanitize=address")

  message(STATUS "Using -fsanitize=${FUZZ_SANITIZER} for Fuzz testing")

  add_subdirectory(fuzz)
endif()

# add headers sanity check target that includes all headers independently
set(check_headers_dir "${PROJECT_BINARY_DIR}/check-headers")
file(GLOB_RECURSE headers_to_check
  ${PROJECT_BINARY_DIR}/*.hpp
  ${PROJECT_SOURCE_DIR}/include/*.hpp)
foreach(header ${headers_to_check})
  if("${header}" MATCHES ".*/include/nodejs/.*")
    # we do not check NodeJS bindings headers
    continue()
  endif()
  get_filename_component(filename ${header} NAME_WE)
  set(filename "${check_headers_dir}/${filename}.cpp")
  if(NOT EXISTS ${filename})
    file(WRITE ${filename} "#include \"${header}\"\n")
  endif()
  list(APPEND sources ${filename})
endforeach()
add_library(check-headers STATIC EXCLUDE_FROM_ALL ${sources})
set_target_properties(check-headers PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${check_headers_dir})

# useful for windows build on ci
push_env("CMAKE_BINARY_DIR" "${CMAKE_BINARY_DIR}")
push_env("PROJECT_SOURCE_DIR" "${PROJECT_SOURCE_DIR}")
push_env("USE_CCACHE" "${USE_CCACHE}")
push_env("EXE" "${CMAKE_EXECUTABLE_SUFFIX}")

message(STATUS "-----------------------")
message(STATUS "Build System is:        ${CMAKE_SYSTEM_NAME}")
message(STATUS "Generator is:           ${CMAKE_GENERATOR}")
message(STATUS "C compiler is:          ${CMAKE_C_COMPILER}")
message(STATUS "CXX compiler is:        ${CMAKE_CXX_COMPILER}")
if(CMAKE_CXX_CLANG_TIDY)
message(STATUS "Clang Tidy is:          ${CMAKE_CXX_CLANG_TIDY}")
endif()
if(CCACHE_BINARY)
message(STATUS "Compiler cache is:      ${CCACHE_BINARY}")
endif()
if(CMAKE_BUILD_TYPE)
message(STATUS "CMAKE_BUILD_TYPE is:    ${CMAKE_BUILD_TYPE}")
endif()
message(STATUS "BUILD_SHARED_LIBS is:   ${BUILD_SHARED_LIBS}")
message(STATUS "BUILD_NODE_PACKAGE is:  ${BUILD_NODE_PACKAGE}")
message(STATUS "LTO / IPO is:           ${CMAKE_INTERPROCEDURAL_OPTIMIZATION}")
message(STATUS "ASSERTIONS are:         ${ENABLE_ASSERTIONS}")
message(STATUS "-----------------------")
message(STATUS "PROJECT_SOURCE_DIR is:        ${PROJECT_SOURCE_DIR}")
message(STATUS "CMAKE_BINARY_DIR is:          ${CMAKE_BINARY_DIR}")
message(STATUS "OSRM_BUILD_DIR is:            $ENV{OSRM_BUILD_DIR}")
message(STATUS "OSRM_UNIT_TESTS_BUILD_DIR is: $ENV{OSRM_UNIT_TESTS_BUILD_DIR}")
message(STATUS "OSRM_BENCHMARKS_BUILD_DIR is: $ENV{OSRM_BENCHMARKS_BUILD_DIR}")
if(BUILD_NODE_PACKAGE)
message(STATUS "OSRM_NODE_JS_BUILD_DIR is:    $ENV{OSRM_NODEJS_BUILD_DIR}")
message(STATUS "OSRM_NODE_JS_INSTALL_DIR is:  $ENV{OSRM_NODEJS_INSTALL_DIR}")
endif()
message(STATUS "OSRM_TEST_DATA_DIR is:        $ENV{OSRM_TEST_DATA_DIR}")
message(STATUS "-----------------------------")
