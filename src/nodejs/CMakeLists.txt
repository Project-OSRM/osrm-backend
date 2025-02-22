cmake_minimum_required(VERSION 3.18)

message(STATUS "Building node_osrm")

set(NAPI_VERSION 8)
add_definitions(-DNAPI_VERSION=${NAPI_VERSION})

set(BINDING_DIR "${PROJECT_SOURCE_DIR}/lib/binding")

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/node_modules/node-cmake")

include(NodeJS)
nodejs_init()

message(STATUS "Configuring node_osrm bindings for NodeJs ${NODEJS_VERSION}")


add_nodejs_module(node_osrm node_osrm.cpp)
set_target_properties(node_osrm PROPERTIES CXX_STANDARD 17)
# TODO: we disable clang-tidy for this target, because it causes errors in third-party NodeJs related headers
set_target_properties(node_osrm PROPERTIES CXX_CLANG_TIDY "")
# TODO: we turn off some warnings for this target, because it causes errors in third-party NodeJs related headers
target_no_warning(node_osrm suggest-destructor-override)
target_no_warning(node_osrm suggest-override)

# https://github.com/cjntaylor/node-cmake/issues/53#issuecomment-842357457
execute_process(COMMAND node -p "require('node-addon-api').include" WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} OUTPUT_VARIABLE NODE_ADDON_API_DIR)
string(REPLACE "\n" "" NODE_ADDON_API_DIR ${NODE_ADDON_API_DIR})
string(REPLACE "\"" "" NODE_ADDON_API_DIR ${NODE_ADDON_API_DIR})

target_include_directories(node_osrm SYSTEM PRIVATE ${NODE_ADDON_API_DIR})


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

# For Conan-enabled builds we copy over tbb's shared objects for packaging.
# TODO: consider using statically linked tbb library (for node_osrm only!)
if (ENABLE_CONAN)
  foreach(libpath ${CONAN_LIB_DIRS_ONETBB})
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
