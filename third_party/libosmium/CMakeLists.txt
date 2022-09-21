#-----------------------------------------------------------------------------
#
#  CMake Config
#
#  Libosmium
#
#-----------------------------------------------------------------------------

cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")


#-----------------------------------------------------------------------------
#
#  Configurations
#
#-----------------------------------------------------------------------------

set(CMAKE_CXX_FLAGS_COVERAGE
    "-g -O0 -fno-inline-functions -fno-inline --coverage ${extra_coverage_flags_}"
    CACHE STRING "Flags used by the compiler during coverage builds.")

set(CMAKE_EXE_LINKER_FLAGS_COVERAGE
    "--coverage"
    CACHE STRING "Flags used by the linker during coverage builds.")

set(CMAKE_CONFIGURATION_TYPES "Debug;Release;RelWithDebInfo;MinSizeRel;Dev;Coverage"
    CACHE STRING
    "List of available configuration types"
    FORCE)


#-----------------------------------------------------------------------------
#
#  Project version
#
#-----------------------------------------------------------------------------

project(libosmium)

set(LIBOSMIUM_VERSION_MAJOR 2)
set(LIBOSMIUM_VERSION_MINOR 15)
set(LIBOSMIUM_VERSION_PATCH 6)

set(LIBOSMIUM_VERSION
    "${LIBOSMIUM_VERSION_MAJOR}.${LIBOSMIUM_VERSION_MINOR}.${LIBOSMIUM_VERSION_PATCH}")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


#-----------------------------------------------------------------------------
#
#  Build options
#
#  (Change with -DOPTION=VALUE on cmake command line.)
#
#-----------------------------------------------------------------------------

if(CMAKE_BUILD_TYPE STREQUAL "Dev")
    set(dev_build ON)
    set(data_test_build ON)
else()
    set(dev_build OFF)
    set(data_test_build OFF)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Coverage")
    set(data_test_build ON)
endif()

option(BUILD_EXAMPLES   "compile example programs" ON)
option(BUILD_TESTING    "compile unit tests, please run them with ctest" ON)

option(BUILD_HEADERS    "compile every header file on its own" ${dev_build})
option(BUILD_BENCHMARKS "compile benchmark programs" ${dev_build})

option(BUILD_DATA_TESTS "compile data tests, please run them with ctest" ${data_test_build})

option(INSTALL_GDALCPP   "also install gdalcpp headers" OFF)

option(WITH_PROFILING    "add flags needed for profiling" OFF)


#-----------------------------------------------------------------------------
#
#  CCache support
#
#-----------------------------------------------------------------------------

option(BUILD_WITH_CCACHE "build using ccache" OFF)

if(BUILD_WITH_CCACHE)
    find_program(CCACHE_PROGRAM ccache)
    if(CCACHE_PROGRAM)
        set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "CCACHE_CPP2=1 ${CCACHE_PROGRAM}")

        # workaround for some clang versions
        if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
            add_definitions(-Qunused-arguments)
        endif()
    endif()
endif()


#-----------------------------------------------------------------------------
#
#  Coverage support
#
#-----------------------------------------------------------------------------

## This leads to all sorts of compile problems, so disable for now
#include(CheckCXXCompilerFlag)
#check_cxx_compiler_flag("-fkeep-inline-functions" HAS_KEEP_INLINE_FUNCTIONS)
#if(HAS_KEEP_INLINE_FUNCTIONS)
#    set(extra_coverage_flags_ "-fkeep-inline-functions")
#endif()

if(CMAKE_BUILD_TYPE STREQUAL "Coverage")
    if(BUILD_EXAMPLES OR BUILD_HEADERS OR BUILD_BENCHMARKS)
        message(WARNING "Coverage builds don't work for anything but the tests")
    endif()

    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        string(REGEX REPLACE "^([0-9]+)\\.([0-9]+).*$" "llvm-cov-\\1.\\2"
               gcov_ ${CMAKE_CXX_COMPILER_VERSION})
    else()
        set(gcov_ "gcov")
    endif()

    find_program(GCOV ${gcov_} DOC "Coverage tool")
    find_program(GCOVR "gcovr" DOC "Coverage report tool")

    set(coverage_report_dir "${CMAKE_BINARY_DIR}/coverage")
    file(MAKE_DIRECTORY ${coverage_report_dir})
    add_custom_target(coverage
        ${GCOVR}
        ${CMAKE_BINARY_DIR}
        --root=${CMAKE_SOURCE_DIR}
        --html --html-details
        #--verbose
        #--keep
        '--filter=.*include/osmium.*'
        --sort-percentage
        --gcov-executable=${GCOV}
        --output=${coverage_report_dir}/index.html)
endif()


#-----------------------------------------------------------------------------
#
#  Find external dependencies
#
#-----------------------------------------------------------------------------

find_package(Boost 1.38)
mark_as_advanced(CLEAR BOOST_ROOT)

if(Boost_FOUND)
    include_directories(SYSTEM ${Boost_INCLUDE_DIRS})
else()
    set(BOOST_ROOT "NOT FOUND: please choose" CACHE PATH "")
    message(FATAL_ERROR "PLEASE, specify the directory where the Boost library is installed in BOOST_ROOT")
endif()

# set OSMIUM_INCLUDE_DIR so FindOsmium will not set anything different
set(OSMIUM_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")

include_directories(${OSMIUM_INCLUDE_DIR})

find_package(Osmium COMPONENTS io gdal geos proj sparsehash)

# The find_package put the directory where it found the libosmium includes
# into OSMIUM_INCLUDE_DIRS. We remove it again, because we want to make
# sure to use our own include directory already set up above.
list(FIND OSMIUM_INCLUDE_DIRS "${OSMIUM_INCLUDE_DIR}" _own_index)
list(REMOVE_AT OSMIUM_INCLUDE_DIRS ${_own_index})
set(_own_index)

include_directories(SYSTEM ${OSMIUM_INCLUDE_DIRS})


#-----------------------------------------------------------------------------
#
#  Decide which C++ version to use (Minimum/default: C++11).
#
#-----------------------------------------------------------------------------
if(NOT MSVC)
    if(NOT USE_CPP_VERSION)
        if(CYGWIN)
            set(USE_CPP_VERSION gnu++11)
        else()
            set(USE_CPP_VERSION c++11)
        endif()
    endif()
    message(STATUS "Use C++ version: ${USE_CPP_VERSION}")
    # following only available from cmake 2.8.12:
    #   add_compile_options(-std=${USE_CPP_VERSION})
    # so using this instead:
    add_definitions(-std=${USE_CPP_VERSION})
endif()


#-----------------------------------------------------------------------------
#
#  Compiler and Linker flags
#
#-----------------------------------------------------------------------------
if(MSVC)
    set(DEV_COMPILE_OPTIONS "/Ox")
    set(RWD_COMPILE_OPTIONS "/Ox /DNDEBUG")
    # do not show warnings caused by missing .pdb files for libraries
    set(USUAL_LINK_OPTIONS "/debug /ignore:4099")
else()
    set(DEV_COMPILE_OPTIONS "-O3 -g")
    set(RWD_COMPILE_OPTIONS "-O3 -g -DNDEBUG")
    set(USUAL_LINK_OPTIONS "")
endif()

if(WIN32)
    add_definitions(-DWIN32 -D_WIN32 -DMSWIN32 -DBGDWIN32)
endif()

set(CMAKE_CXX_FLAGS_DEV "${DEV_COMPILE_OPTIONS}"
    CACHE STRING "Flags used by the compiler during developer builds."
    FORCE)

set(CMAKE_EXE_LINKER_FLAGS_DEV "${USUAL_LINK_OPTIONS}"
    CACHE STRING "Flags used by the linker during developer builds."
    FORCE)
mark_as_advanced(
    CMAKE_CXX_FLAGS_DEV
    CMAKE_EXE_LINKER_FLAGS_DEV
)

set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${RWD_COMPILE_OPTIONS}"
    CACHE STRING "Flags used by the compiler during RELWITHDEBINFO builds."
    FORCE)

set(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO "${USUAL_LINK_OPTIONS}"
    CACHE STRING "Flags used by the linker during RELWITHDEBINFO builds."
    FORCE)

mark_as_advanced(
    CMAKE_CXX_FLAGS_RELWITHDEBINFO
    CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO
)

if(WITH_PROFILING)
    add_definitions(-fno-omit-frame-pointer)
endif()


#-----------------------------------------------------------------------------
#
#  Build Type
#
#-----------------------------------------------------------------------------

# In 'Dev' mode: compile with very strict warnings and turn them into errors.
if(CMAKE_BUILD_TYPE STREQUAL "Dev")
    if(NOT MSVC)
        add_definitions(-Werror)
    endif()
    add_definitions(${OSMIUM_WARNING_OPTIONS})
#    add_definitions(${OSMIUM_WARNING_OPTIONS} ${OSMIUM_DRACONIC_CLANG_OPTIONS} -Wno-documentation -Wno-format-nonliteral -Wno-deprecated -Wno-covered-switch-default -Wno-shadow)
endif()

# Force RelWithDebInfo build type if none was given
if(CMAKE_BUILD_TYPE)
    set(build_type ${CMAKE_BUILD_TYPE})
else()
    set(build_type "RelWithDebInfo")
endif()

set(CMAKE_BUILD_TYPE ${build_type}
    CACHE STRING
    "Choose the type of build, options are: ${CMAKE_CONFIGURATION_TYPES}."
    FORCE)


#-----------------------------------------------------------------------------
#
#  Unit and data tests
#
#-----------------------------------------------------------------------------
enable_testing()

if(BUILD_TESTING OR BUILD_DATA_TESTS)
    find_program(MEMORYCHECK_COMMAND valgrind)

    set(MEMORYCHECK_COMMAND_OPTIONS
        "--trace-children=yes --leak-check=full --show-reachable=yes --error-exitcode=1")

    set(MEMORYCHECK_SUPPRESSIONS_FILE "${PROJECT_SOURCE_DIR}/test/valgrind.supp")
endif()

if(BUILD_TESTING)
    add_subdirectory(test)
endif()

if(BUILD_DATA_TESTS)
    add_subdirectory(test/data-tests)
endif()

if(BUILD_EXAMPLES)
    add_subdirectory(test/examples)
endif()


#-----------------------------------------------------------------------------
#
#  Optional "cppcheck" target that checks C++ code
#
#-----------------------------------------------------------------------------
message(STATUS "Looking for cppcheck")
find_program(CPPCHECK cppcheck)

if(CPPCHECK)
    message(STATUS "Looking for cppcheck - found")
    set(CPPCHECK_OPTIONS
        --language=c++
        --quiet
        -j4
        --inline-suppr
        --enable=warning,style,performance,portability,information,missingInclude
        --force
        -Uassert -DPROTOZERO_STRICT_API -DPROTOZERO_USE_BUILTIN_BSWAP -UPROTOZERO_USE_VIEW)

    # cpp doesn't find system includes for some reason, suppress that report
    set(CPPCHECK_OPTIONS ${CPPCHECK_OPTIONS} --suppress=missingIncludeSystem)

    file(GLOB_RECURSE ALL_INCLUDES   include/osmium/*.hpp)
    file(GLOB         ALL_EXAMPLES   examples/*.cpp)
    file(GLOB         ALL_BENCHMARKS benchmarks/*.cpp)
    file(GLOB         ALL_UNIT_TESTS test/t/*/test_*.cpp)
    file(GLOB         ALL_DATA_TESTS test/data-tests/*.cpp)

    if(Osmium_DEBUG)
        message(STATUS "Checking includes      : ${ALL_INCLUDES}")
        message(STATUS "Checking example code  : ${ALL_EXAMPLES}")
        message(STATUS "Checking benchmarks    : ${ALL_BENCHMARKS}")
        message(STATUS "Checking unit test code: ${ALL_UNIT_TESTS}")
        message(STATUS "Checking data test code: ${ALL_DATA_TESTS}")
    endif()

    set(CPPCHECK_FILES
        ${ALL_INCLUDES}
        ${ALL_EXAMPLES}
        ${ALL_BENCHMARKS}
        ${ALL_UNIT_TESTS}
        ${ALL_DATA_TESTS})

    add_custom_target(cppcheck
        ${CPPCHECK}
        --std=c++11 ${CPPCHECK_OPTIONS}
        -I ${CMAKE_SOURCE_DIR}/include
        ${CPPCHECK_FILES}
    )
else()
    message(STATUS "Looking for cppcheck - not found")
    message(STATUS "  Build target 'cppcheck' will not be available.")
endif()


#-----------------------------------------------------------------------------
#
#  Examples, benchmarks and documentation
#
#-----------------------------------------------------------------------------

if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if(BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()

add_subdirectory(doc)


#-----------------------------------------------------------------------------
#
#  Headers
#
#  This will try to compile include files on their own to detect missing
#  include directives and other dependency-related problems. Note that if this
#  work, it is not enough to be sure it will compile in production code.
#  But if it reports an error we know we are missing something.
#
#-----------------------------------------------------------------------------
if(BUILD_HEADERS)
    file(GLOB_RECURSE
         ALL_HPPS
         RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}/include"
         include/osmium/*.hpp)

    file(MAKE_DIRECTORY header_check)

    foreach(hpp ${ALL_HPPS})
        if(GDAL_FOUND OR NOT ((hpp STREQUAL "osmium/area/problem_reporter_ogr.hpp") OR (hpp STREQUAL "osmium/geom/ogr.hpp")))
            string(REPLACE ".hpp" "" tmp ${hpp})
            string(REPLACE "/" "__" libname ${tmp})

            # Create a dummy .cpp file that includes the header file we want to
            # check.
            set(DUMMYCPP ${CMAKE_BINARY_DIR}/header_check/${libname}.cpp)
            file(WRITE ${DUMMYCPP} "#define OSMIUM_UTIL_COMPATIBILITY_HPP\n#define OSMIUM_DEPRECATED\n#include <${hpp}> // IWYU pragma: keep\n")

            # There is no way in CMake to just compile but not link a C++ file,
            # so we pretend to build a library here.
            add_library(${libname} STATIC ${DUMMYCPP} include/${hpp})

            #### this is better but only supported from cmake 3.0:
            ###add_library(${libname} OBJECT ${DUMMYCPP} include/${hpp})
        endif()
    endforeach()
endif()


#-----------------------------------------------------------------------------
#
#  Optional "clang-tidy" target
#
#-----------------------------------------------------------------------------
message(STATUS "Looking for clang-tidy")
find_program(CLANG_TIDY NAMES clang-tidy clang-tidy-10 clang-tidy-9 clang-tidy-8 clang-tidy-7 clang-tidy-7.0 clang-tidy-6.0 clang-tidy-5.0)

if(CLANG_TIDY)
    message(STATUS "Looking for clang-tidy - found ${CLANG_TIDY}")

    if(BUILD_EXAMPLES)
        file(GLOB CT_ALL_EXAMPLES examples/*.cpp)
    endif()

    if(BUILD_TESTING)
        file(GLOB CT_ALL_UNIT_TESTS test/t/*/test_*.cpp)
    endif()

    if(BUILD_HEADERS)
        file(GLOB_RECURSE CT_ALL_INCLUDES ${CMAKE_BINARY_DIR}/header_check/osmium__*.cpp)
    endif()

    if(BUILD_BENCHMARKS)
        file(GLOB CT_ALL_BENCHMARKS benchmarks/*.cpp)
    endif()

    if(BUILD_DATA_TESTS)
        file(GLOB CT_ALL_DATA_TESTS test/data-tests/*.cpp)
    endif()

    if(Osmium_DEBUG)
        message(STATUS "Checking example code  : ${CT_ALL_EXAMPLES}")
        message(STATUS "Checking unit test code: ${CT_ALL_UNIT_TESTS}")
        message(STATUS "Checking includes      : ${CT_ALL_INCLUDES}")
        message(STATUS "Checking benchmarks    : ${CT_ALL_BENCHMARKS}")
        message(STATUS "Checking data test code: ${CT_ALL_DATA_TESTS}")
    endif()

    set(CT_CHECK_FILES
        ${CT_ALL_EXAMPLES}
        ${CT_ALL_UNIT_TESTS}
        ${CT_ALL_INCLUDES}
        ${CT_ALL_BENCHMARKS}
        ${CT_ALL_DATA_TESTS})

    add_custom_target(clang-tidy
        ${CLANG_TIDY}
        -p ${CMAKE_BINARY_DIR}
        "-extra-arg=-Wno-#pragma-messages"
        ${CT_CHECK_FILES}
    )
else()
    message(STATUS "Looking for clang-tidy - not found")
    message(STATUS "  Build target 'clang-tidy' will not be available.")
endif()

#-----------------------------------------------------------------------------
#
#  Installation
#
#  External libraries are only installed if the options are set in case they
#  are installed from somewhere else.
#
#-----------------------------------------------------------------------------
install(DIRECTORY include/osmium DESTINATION include)

if(INSTALL_GDALCPP)
    install(FILES include/gdalcpp.hpp DESTINATION include)
endif()


#-----------------------------------------------------------------------------
#
#  Packaging
#
#-----------------------------------------------------------------------------

set(CPACK_PACKAGE_VERSION_MAJOR ${LIBOSMIUM_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${LIBOSMIUM_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${LIBOSMIUM_VERSION_PATCH})

if(WIN32)
    set(CPACK_GENERATOR ZIP)
else()
    set(CPACK_GENERATOR TGZ)
endif()

include(CPack)


#-----------------------------------------------------------------------------
#
#  Print warnings at the end
#
#-----------------------------------------------------------------------------
if(BUILD_DATA_TESTS AND OSM_TESTDATA STREQUAL "OSM_TESTDATA-NOTFOUND")
    message("\n========================== WARNING ==========================")
    message("osm-testdata directory not found, data tests were disabled!\n")
    message("Call 'git submodule update --init' to install test data")
    message("or set the OSM_TESTDATA cmake variable to its path.")
    message("=============================================================\n")
endif()

#-----------------------------------------------------------------------------
