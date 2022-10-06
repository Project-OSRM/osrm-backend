#-----------------------------------------------------------------------------
#
#  CMake config
#
#  protozero
#
#-----------------------------------------------------------------------------

cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

#-----------------------------------------------------------------------------

project(protozero)

set(PROTOZERO_VERSION_MAJOR 1)
set(PROTOZERO_VERSION_MINOR 7)
set(PROTOZERO_VERSION_PATCH 0)

set(PROTOZERO_VERSION
    "${PROTOZERO_VERSION_MAJOR}.${PROTOZERO_VERSION_MINOR}.${PROTOZERO_VERSION_PATCH}")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#-----------------------------------------------------------------------------

option(WERROR "Add -Werror flag to build (turns warnings into errors)" ON)

if(MSVC)
    add_definitions(/W3)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS)
else()
    add_definitions(-std=c++11 -Wall -Wextra -pedantic -Wsign-compare -Wunused-parameter -Wno-float-equal -Wno-covered-switch-default)
    if(WERROR)
        add_definitions(-Werror)
    endif()
endif()

include_directories("${CMAKE_SOURCE_DIR}/include")

set(PROTOZERO_DATA_VIEW "" CACHE STRING "Type used for protozero::data_view")
if(NOT PROTOZERO_DATA_VIEW STREQUAL "")
    add_definitions(-DPROTOZERO_DATA_VIEW=${PROTOZERO_DATA_VIEW})
endif()


#-----------------------------------------------------------------------------
#
#  Find dependencies
#
#-----------------------------------------------------------------------------

find_package(Protobuf)


#-----------------------------------------------------------------------------
#
#  Optional "clang-tidy" target
#
#-----------------------------------------------------------------------------
message(STATUS "Looking for clang-tidy")
find_program(CLANG_TIDY NAMES clang-tidy clang-tidy-10 clang-tidy-9 clang-tidy-8 clang-tidy-7 clang-tidy-6.0 clang-tidy-5.0)

if(CLANG_TIDY)
    message(STATUS "Looking for clang-tidy - found ${CLANG_TIDY}")
    add_custom_target(clang-tidy
        ${CLANG_TIDY}
        -p ${CMAKE_BINARY_DIR}
        ${CMAKE_SOURCE_DIR}/test/*.cpp
        ${CMAKE_SOURCE_DIR}/test/t/*/reader_test_cases.cpp
        ${CMAKE_SOURCE_DIR}/test/t/*/writer_test_cases.cpp
        ${CMAKE_SOURCE_DIR}/test/unit/*.cpp
        ${CMAKE_SOURCE_DIR}/tools/*.cpp
    )
    add_dependencies(clang-tidy writer_tests)
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
        ${CMAKE_SOURCE_DIR}/include/protozero/*.hpp
        ${CMAKE_SOURCE_DIR}/test/*.cpp
        ${CMAKE_SOURCE_DIR}/test/include/*.hpp
        ${CMAKE_SOURCE_DIR}/test/t/*/*.cpp
        ${CMAKE_SOURCE_DIR}/test/unit/*.cpp
        ${CMAKE_SOURCE_DIR}/tools/*.cpp
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

install(DIRECTORY include/protozero DESTINATION include)


#-----------------------------------------------------------------------------

enable_testing()

add_subdirectory(doc)

add_subdirectory(tools)

add_subdirectory(test)


#-----------------------------------------------------------------------------
