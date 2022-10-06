#-----------------------------------------------------------------------------
#
#  CMake config
#
#  vtzero
#
#-----------------------------------------------------------------------------

cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

#-----------------------------------------------------------------------------

project(vtzero)

set(VTZERO_VERSION_MAJOR 1)
set(VTZERO_VERSION_MINOR 0)
set(VTZERO_VERSION_PATCH 1)

set(VTZERO_VERSION
    "${VTZERO_VERSION_MAJOR}.${VTZERO_VERSION_MINOR}.${VTZERO_VERSION_PATCH}")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#-----------------------------------------------------------------------------

# This variable must be set to the directory where the mvt-fixtures from the
# https://github.com/mapbox/mvt-fixtures repository are to be found. Usually
# this is the directory where the submodule is checked out as described in
# the README, but you can also set this to a different path, for instance
# to change the setting while doing development.

set(MVT_FIXTURES "${CMAKE_SOURCE_DIR}/test/mvt-fixtures" CACHE PATH "mvt-fixtures directory for tests")

#-----------------------------------------------------------------------------

option(WERROR "Add -Werror flag to build (turns warnings into errors)" ON)

if(MSVC)
    add_definitions(-std=c++11 /W3)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS)
else()
    add_definitions(-std=c++11 -Wall -Wextra -pedantic -Wsign-compare -Wconversion)
#    add_definitions(-Weverything -Wno-c++98-compat -Wno-documentation -Wno-switch-enum -Wno-weak-vtables -Wno-padded -Wno-documentation-unknown-command -Wno-exit-time-destructors)
    if(WERROR)
        add_definitions(-Werror)
    endif()
endif()

include_directories("${CMAKE_SOURCE_DIR}/include")

set(PROTOZERO_DATA_VIEW "" CACHE STRING "Type used for vtzero::data_view")
if(NOT PROTOZERO_DATA_VIEW STREQUAL "")
    add_definitions(-DPROTOZERO_DATA_VIEW=${PROTOZERO_DATA_VIEW})
endif()


#-----------------------------------------------------------------------------
#
#  Find dependencies
#
#-----------------------------------------------------------------------------

find_package(Protozero 1.6.0 REQUIRED)

include_directories(SYSTEM ${PROTOZERO_INCLUDE_DIR})

find_package(Boost)


#-----------------------------------------------------------------------------
#
#  Optional "clang-tidy" target
#
#-----------------------------------------------------------------------------
message(STATUS "Looking for clang-tidy")
find_program(CLANG_TIDY NAMES clang-tidy clang-tidy-6.0 clang-tidy-5.0)

if(CLANG_TIDY)
    message(STATUS "Looking for clang-tidy - found ${CLANG_TIDY}")
    add_custom_target(clang-tidy
        ${CLANG_TIDY}
        -p ${CMAKE_BINARY_DIR}
        ${CMAKE_SOURCE_DIR}/examples/*.cpp
        ${CMAKE_SOURCE_DIR}/test/*.cpp
        ${CMAKE_SOURCE_DIR}/test/t/*.cpp
    )
else()
    message(STATUS "Looking for clang-tidy - not found")
    message(STATUS "  Build target 'clang-tidy' will not be available.")
endif()


#-----------------------------------------------------------------------------
#
#  Optional "cppcheck" target
#
#-----------------------------------------------------------------------------
message(STATUS "Looking for cppcheck")
find_program(CPPCHECK NAMES cppcheck)

if(CPPCHECK)
    message(STATUS "Looking for cppcheck - found")
    add_custom_target(cppcheck
        ${CPPCHECK}
        -Uassert --std=c++11 --enable=all
        ${CMAKE_SOURCE_DIR}/examples/*.cpp
        ${CMAKE_SOURCE_DIR}/test/*.cpp
        ${CMAKE_SOURCE_DIR}/test/t/*.cpp
    )
else()
    message(STATUS "Looking for cppcheck - not found")
    message(STATUS "  Build target 'cppcheck' will not be available.")
endif()


#-----------------------------------------------------------------------------
#
#  Include what you use
#
#-----------------------------------------------------------------------------
message(STATUS "Looking for iwyu")
find_program(IWYU_TOOL NAMES iwyu_tool)

if(IWYU_TOOL)
    message(STATUS "Looking for iwyu - found")
    add_custom_target(iwyu
        ${IWYU_TOOL} -p ${CMAKE_BINARY_DIR}
    )
else()
    message(STATUS "Looking for iwyu - not found")
    message(STATUS "  Build target 'iwyu' will not be available.")
endif()


#-----------------------------------------------------------------------------
#
#  Installation
#
#-----------------------------------------------------------------------------

install(DIRECTORY include/vtzero DESTINATION include)


#-----------------------------------------------------------------------------

enable_testing()

add_subdirectory(doc)

add_subdirectory(examples)

add_subdirectory(test)


#-----------------------------------------------------------------------------
