cmake_minimum_required(VERSION 3.18)

message(STATUS "Building node_osrm")

set(NAPI_VERSION 8)
add_definitions(-DNAPI_VERSION=${NAPI_VERSION})

set(BINDING_DIR "${PROJECT_SOURCE_DIR}/lib/binding_napi_v8")

# cmake-js sets CMAKE_JS_INC, CMAKE_JS_LIB, CMAKE_JS_SRC and CMAKE_JS_NODELIB_*
# when cmake is invoked via `npx cmake-js configure`. When cmake is invoked
# directly (e.g. on Linux/macOS CI), we fall back to querying cmake-js for the
# Node.js headers and any platform-specific sources.
if(NOT CMAKE_JS_INC)
    execute_process(
        COMMAND node "${PROJECT_SOURCE_DIR}/node_modules/cmake-js/bin/cmake-js"
                --log-level error print-cmakejs-include
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
        OUTPUT_VARIABLE CMAKE_JS_INC
        OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE _cmake_js_result
    )
    if(NOT _cmake_js_result EQUAL 0)
        message(FATAL_ERROR
            "cmake-js failed to provide Node.js include paths (exit ${_cmake_js_result}). "
            "Make sure cmake-js is installed: npm install")
    endif()
endif()

if(NOT CMAKE_JS_LIB)
    execute_process(
        COMMAND node "${PROJECT_SOURCE_DIR}/node_modules/cmake-js/bin/cmake-js"
                --log-level error print-cmakejs-lib
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
        OUTPUT_VARIABLE CMAKE_JS_LIB
        OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE _cmake_js_lib_result
        ERROR_VARIABLE _cmake_js_lib_error
    )
    if(NOT _cmake_js_lib_result EQUAL 0)
        message(FATAL_ERROR
            "cmake-js failed to provide Node.js library path (exit ${_cmake_js_lib_result}). "
            "Make sure cmake-js is installed: npm install. "
            "${_cmake_js_lib_error}")
    endif()
endif()

if(NOT CMAKE_JS_SRC)
    execute_process(
        COMMAND node "${PROJECT_SOURCE_DIR}/node_modules/cmake-js/bin/cmake-js"
                --log-level error print-cmakejs-src
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
        OUTPUT_VARIABLE CMAKE_JS_SRC
        ERROR_VARIABLE _cmake_js_src_error
        OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE _cmake_js_src_result
    )
    if(NOT _cmake_js_src_result EQUAL 0)
        message(FATAL_ERROR
            "cmake-js failed to provide Node.js source paths (exit ${_cmake_js_src_result}). "
            "stderr: ${_cmake_js_src_error}. "
            "Make sure cmake-js is installed: npm install")
    endif()
endif()

message(STATUS "Configuring node_osrm bindings")

# On Windows/MSVC cmake-js needs a node.lib stub to link against.
# cmake-js provides CMAKE_JS_NODELIB_DEF and CMAKE_JS_NODELIB_TARGET when it
# drives the build; when cmake is invoked directly we derive them ourselves.
if(MSVC)
    if(NOT CMAKE_JS_NODELIB_DEF)
        execute_process(
            COMMAND node "${PROJECT_SOURCE_DIR}/cmake/get-node-api-def.cjs"
            WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
            OUTPUT_VARIABLE CMAKE_JS_NODELIB_DEF
            OUTPUT_STRIP_TRAILING_WHITESPACE
            RESULT_VARIABLE _node_api_def_result
        )
        if(NOT _node_api_def_result EQUAL 0)
            message(FATAL_ERROR
                "Failed to locate node_api.def (exit ${_node_api_def_result}). "
                "Make sure cmake-js is installed: npm install")
        endif()
        file(TO_CMAKE_PATH "${CMAKE_JS_NODELIB_DEF}" CMAKE_JS_NODELIB_DEF)
    endif()
    if(NOT CMAKE_JS_NODELIB_TARGET)
        set(CMAKE_JS_NODELIB_TARGET "${CMAKE_CURRENT_BINARY_DIR}/node.lib")
    endif()
    if(CMAKE_JS_NODELIB_DEF AND CMAKE_JS_NODELIB_TARGET)
        execute_process(
            COMMAND ${CMAKE_AR} /def:${CMAKE_JS_NODELIB_DEF}
                                /out:${CMAKE_JS_NODELIB_TARGET}
            RESULT_VARIABLE _nodelib_result
        )
        if(NOT _nodelib_result EQUAL 0)
            message(FATAL_ERROR
                "Failed to generate node.lib from ${CMAKE_JS_NODELIB_DEF} "
                "(exit ${_nodelib_result})")
        endif()
    endif()
    # Always use the locally-generated node.lib on MSVC. cmake-js's
    # print-cmakejs-lib returns <workDir>/node.lib (the root build dir), but
    # when cmake is invoked directly the lib is generated in
    # CMAKE_CURRENT_BINARY_DIR instead, so we must override any earlier value.
    set(CMAKE_JS_LIB "${CMAKE_JS_NODELIB_TARGET}")
endif()

add_library(node_osrm SHARED node_osrm.cpp ${CMAKE_JS_SRC})
set_target_properties(node_osrm PROPERTIES
    PREFIX ""
    SUFFIX ".node"
    CXX_STANDARD 20)
# TODO: we disable clang-tidy for this target, because it causes errors in third-party NodeJs related headers
set_target_properties(node_osrm PROPERTIES CXX_CLANG_TIDY "")
# TODO: we turn off some warnings for this target, because it causes errors in third-party NodeJs related headers
target_no_warning(node_osrm suggest-destructor-override)
target_no_warning(node_osrm suggest-override)

# CMAKE_JS_INC already contains the node-addon-api include path when cmake-js
# detects binary.napi_versions in package.json.
target_include_directories(node_osrm SYSTEM PRIVATE ${CMAKE_JS_INC})

target_link_libraries(node_osrm ${CMAKE_JS_LIB} osrm)

# On macOS, Node-API symbols are resolved from the Node.js process at load
# time, not from a static or shared library. Tell the linker not to error on
# unresolved symbols so the .node bundle can be loaded by node at runtime.
# cmake-js sets this flag automatically when it drives the build via
# `cmake-js configure`; we mirror it here for direct cmake invocations.
if(APPLE)
    target_link_options(node_osrm PRIVATE -undefined dynamic_lookup)
endif()

# node_osrm artifacts in ${BINDING_DIR} to depend targets on
set(ARTIFACTS "")

# Create the binding directory if it doesn't exist
file(MAKE_DIRECTORY ${BINDING_DIR})

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
if(BUILD_AS_SUBPROJECT)
  add_custom_target(osrm_copy_artifacts ALL DEPENDS ${ARTIFACTS})
else()
  add_custom_target(copy_artifacts ALL DEPENDS ${ARTIFACTS})
endif()
