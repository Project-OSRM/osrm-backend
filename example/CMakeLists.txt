cmake_minimum_required(VERSION 3.18)

if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_CURRENT_BINARY_DIR AND NOT MSVC_IDE)
  message(FATAL_ERROR "In-source builds are not allowed.
Please create a directory and run cmake from there, passing the path to this source directory as the last argument.
This process created the file `CMakeCache.txt' and the directory `CMakeFiles'. Please delete them.")
endif()

if(NOT CMAKE_BUILD_TYPE MATCHES Debug)
  set(CMAKE_BUILD_TYPE Release)
endif()

project(osrm-example C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# find_package(LibOSRM) resolves via the CMake Config package installed
# at <prefix>/lib/cmake/LibOSRM/ (LibOSRMConfig.cmake). That package
# re-declares Boost / TBB / Threads / ZLIB as find_dependency() calls,
# so consumers do not have to call find_package() for them by hand.
find_package(LibOSRM REQUIRED)

add_executable(osrm-example example.cpp)
target_link_libraries(osrm-example PRIVATE osrm::osrm)
