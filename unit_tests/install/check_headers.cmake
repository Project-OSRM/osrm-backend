set(staging_prefix "${OSRM_BUILD_DIR}/public-header-test/install")
file(REMOVE_RECURSE "${staging_prefix}")

execute_process(
    COMMAND "${CMAKE_COMMAND}" --install "${OSRM_BUILD_DIR}"
        --prefix "${staging_prefix}" --component Development
    RESULT_VARIABLE result)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "Cannot install public headers: ${result}")
endif()

execute_process(
    COMMAND "${CMAKE_COMMAND}"
        -S "${OSRM_SOURCE_DIR}/unit_tests/install"
        -B "${OSRM_BUILD_DIR}/public-header-test/consumer"
        "-DCMAKE_CXX_COMPILER=${OSRM_CXX_COMPILER}"
        "-DOSRM_INSTALL_PREFIX=${staging_prefix}"
    RESULT_VARIABLE result)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "Cannot configure installed-header consumer: ${result}")
endif()

execute_process(
    COMMAND "${CMAKE_COMMAND}" --build "${OSRM_BUILD_DIR}/public-header-test/consumer"
    RESULT_VARIABLE result)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "Installed preprocessing headers do not compile: ${result}")
endif()
