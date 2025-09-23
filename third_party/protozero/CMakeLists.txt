#-----------------------------------------------------------------------------
#
#  CMake config
#
#  protozero
#
#-----------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.10.0 FATAL_ERROR)

#-----------------------------------------------------------------------------

project(protozero VERSION 1.8.1 LANGUAGES CXX C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#-----------------------------------------------------------------------------

if (NOT "${CMAKE_CXX_STANDARD}")
    set(CMAKE_CXX_STANDARD 14)
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

# Used for testing
set(PROTOZERO_DATA_VIEW "" CACHE STRING "Type used for protozero::data_view")
if(NOT PROTOZERO_DATA_VIEW STREQUAL "")
    message(STATUS "Using ${PROTOZERO_DATA_VIEW} as data_view")
    add_definitions(-DPROTOZERO_USE_VIEW=${PROTOZERO_DATA_VIEW})
else()
    message(STATUS "Using built-in data_view")
endif()


#-----------------------------------------------------------------------------
#
#  Find dependencies
#
#-----------------------------------------------------------------------------

if(BUILD_TESTING)
    # This is needed for the protobuf_generate_cpp() function to work on newer
    # versions of the Protobuf CMake config.
    set(protobuf_MODULE_COMPATIBLE ON CACHE BOOL "")

    # This is needed so that we pick up the (more modern) CONFIG mode cmake file
    # before the (older) MODULE mode cmake config file. Seems to be needed on macOS.
    set(CMAKE_FIND_PACKAGE_PREFER_CONFIG TRUE)

    find_package(Protobuf)
endif()

#-----------------------------------------------------------------------------
#
#  Optional "clang-tidy" target
#
#-----------------------------------------------------------------------------
message(STATUS "Looking for clang-tidy")
find_program(CLANG_TIDY NAMES clang-tidy-20 clang-tidy-19 clang-tidy-18 clang-tidy-17 clang-tidy-16 clang-tidy-15)

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
    if(BUILD_TESTING)
        add_dependencies(clang-tidy writer_tests)
    endif()
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
        -Uassert --std=c++14 --enable=all
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
