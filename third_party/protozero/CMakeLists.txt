#-----------------------------------------------------------------------------
#
#  CMake config
#
#  protozero
#
#-----------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.5.0 FATAL_ERROR)

#-----------------------------------------------------------------------------

project(protozero VERSION 1.7.1 LANGUAGES CXX C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#-----------------------------------------------------------------------------

if (NOT "${CMAKE_CXX_STANDARD}")
    set(CMAKE_CXX_STANDARD 11)
endif()
message(STATUS "Building in C++${CMAKE_CXX_STANDARD} mode")
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#-----------------------------------------------------------------------------

option(BUILD_TESTING "Build tests" ON)
option(WERROR "Add -Werror flag to build (turns warnings into errors)" ON)

if(MSVC)
    add_compile_options(/W3)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS)
else()
    add_compile_options(-Wall -Wextra -pedantic -Wsign-compare -Wunused-parameter -Wno-float-equal -Wno-covered-switch-default)
    if(WERROR)
        add_compile_options(-Werror)
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
find_program(CLANG_TIDY NAMES clang-tidy clang-tidy-14 clang-tidy-13 clang-tidy-12 clang-tidy-11)

if(CLANG_TIDY AND PROTOBUF_FOUND)
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

add_subdirectory(doc)

if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(test)
endif()

add_subdirectory(tools)

#-----------------------------------------------------------------------------
