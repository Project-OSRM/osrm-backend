# node-cmake requires CMake 3.1 features; for the osrm project we only
# require CMake 2.8.11 so that we can build e.g. on Trusty by default.
cmake_minimum_required(VERSION 3.1)

message(STATUS "Building node-osrm")

set(BINDING_DIR "${PROJECT_SOURCE_DIR}/lib/binding")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/nodejs")
include(FindNodeJS)

set(NodeJS_CXX_STANDARD 14 CACHE INTERNAL "Use C++14" FORCE)
set(NodeJS_DOWNLOAD ON CACHE INTERNAL "Download node.js sources" FORCE)
set(NodeJS_USE_CLANG_STDLIB OFF CACHE BOOL "Don't use libc++ by default" FORCE)

find_package(NodeJS REQUIRED)
add_nodejs_module(node-osrm node_osrm.cpp)
target_link_libraries(node-osrm osrm)

# node-osrm artifacts in ${BINDING_DIR} to depend targets on
set(ARTIFACTS "")

set(OSRM_BINARIES osrm-extract osrm-contract osrm-routed osrm-datastore osrm-components)
foreach(binary ${OSRM_BINARIES})
  add_custom_command(OUTPUT ${BINDING_DIR}/${binary}
                     COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${binary}> ${BINDING_DIR}
                     DEPENDS ${binary} ${BINDING_DIR})
  list(APPEND ARTIFACTS "${BINDING_DIR}/${binary}")
endforeach(binary)

# For mason-enabled builds we copy over tbb's shared objects for packaging.
# TODO: consider using statically linked tbb library (for node-osrm only!)
if (ENABLE_MASON)
  foreach(libpath ${MASON_PACKAGE_tbb_LIBRARY_DIRS})
    file(GLOB TBBGlob ${libpath}/*.*)
    foreach(filepath ${TBBGlob})
      get_filename_component(filename ${filepath} NAME)
      add_custom_command(OUTPUT "${BINDING_DIR}/${filename}"
                         COMMAND ${CMAKE_COMMAND} -E copy ${filepath} ${BINDING_DIR}
                         DEPENDS ${filepath} ${BINDING_DIR})
      list(APPEND ARTIFACTS "${BINDING_DIR}/${filename}")
    endforeach()
  endforeach()
endif()


add_custom_command(OUTPUT ${BINDING_DIR}/node-osrm.node
                   COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:node-osrm> ${BINDING_DIR}
                   DEPENDS node-osrm ${BINDING_DIR})
list(APPEND ARTIFACTS "${BINDING_DIR}/node-osrm.node")


message(STATUS "node-osrm artifacts will be copied to: ${BINDING_DIR}")
add_custom_target(copy_artifacts ALL DEPENDS ${ARTIFACTS})
