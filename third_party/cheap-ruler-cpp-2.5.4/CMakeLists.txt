cmake_minimum_required(VERSION 3.0.0 FATAL_ERROR)
project(cheap_ruler LANGUAGES CXX C)

include(cmake/build.cmake)
include(cmake/mason.cmake)

set(CMAKE_OSX_DEPLOYMENT_TARGET 10.10)
set(CMAKE_CONFIGURATION_TYPES Debug Release)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -Wall -Wextra -Wpedantic -Wshadow")

mason_use(geometry VERSION 0.9.2 HEADER_ONLY)
mason_use(gtest VERSION 1.8.0)
mason_use(variant VERSION 1.1.4 HEADER_ONLY)

add_definitions(-D_GLIBCXX_USE_CXX11_ABI=0)

add_executable(cheap_ruler
    ${PROJECT_SOURCE_DIR}/test/cheap_ruler.cpp
)

target_include_directories(cheap_ruler
    PUBLIC ${PROJECT_SOURCE_DIR}/include
)

target_add_mason_package(cheap_ruler PRIVATE geometry)
target_add_mason_package(cheap_ruler PRIVATE gtest)
target_add_mason_package(cheap_ruler PRIVATE variant)
