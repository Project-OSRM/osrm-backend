# node-cmake requires CMake 3.1 features; for the osrm project we only
# require CMake 2.8.11 so that we can build e.g. on Trusty by default.
cmake_minimum_required(VERSION 3.1)

message(STATUS "Building node_osrm")

set(BINDING_DIR "${PROJECT_SOURCE_DIR}/lib/binding")

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/node_modules/node-cmake")

include(NodeJS)
nodejs_init()

message(STATUS "Configuring node_osrm bindings for NodeJs ${NODEJS_VERSION}")

add_nodejs_module(node_osrm node_osrm.cpp)
set_target_properties(node_osrm PROPERTIES CXX_STANDARD 14)
target_link_libraries(node_osrm osrm)

# node_osrm artifacts in ${BINDING_DIR} to depend targets on
set(ARTIFACTS "")

set(OSRM_BINARIES osrm-extract osrm-contract osrm-routed osrm-datastore osrm-components osrm-partition osrm-customize)
foreach(binary ${OSRM_BINARIES})
  add_custom_command(OUTPUT ${BINDING_DIR}/${binary}
                     COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${binary}> ${BINDING_DIR}
                     DEPENDS ${binary} ${BINDING_DIR})
  list(APPEND ARTIFACTS "${BINDING_DIR}/${binary}")
endforeach(binary)

# For mason-enabled builds we copy over tbb's shared objects for packaging.
# TODO: consider using statically linked tbb library (for node_osrm only!)
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


add_custom_command(OUTPUT ${BINDING_DIR}/node_osrm.node
                   COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:node_osrm> ${BINDING_DIR}
                   DEPENDS node_osrm ${BINDING_DIR})
list(APPEND ARTIFACTS "${BINDING_DIR}/node_osrm.node")


message(STATUS "node_osrm artifacts will be copied to: ${BINDING_DIR}")
add_custom_target(copy_artifacts ALL DEPENDS ${ARTIFACTS})
