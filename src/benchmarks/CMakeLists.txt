set(TEST_DATA_DIR $ENV{OSRM_TEST_DATA_DIR})
set(LOG_DIR "${PROJECT_SOURCE_DIR}/test/logs")

set(MONACO "${TEST_DATA_DIR}")
set(BERLIN "${TEST_DATA_DIR}")
set(GPS_TRACES "${TEST_DATA_DIR}/berlin_gps_traces.csv.gz")

mk_build_dir("OSRM_BENCHMARKS_BUILD_DIR" "${CMAKE_CURRENT_BINARY_DIR}")
push_env("OSRM_BENCHMARKS_BUILD_DIR" "${OSRM_BENCHMARKS_BUILD_DIR}")

### build the benchmarks ###

add_executable(alias-bench        EXCLUDE_FROM_ALL alias.cpp)
add_executable(bench	          EXCLUDE_FROM_ALL bench.cpp)
add_executable(json-render-bench  EXCLUDE_FROM_ALL json_render.cpp)
add_executable(match-bench        EXCLUDE_FROM_ALL match.cpp)
add_executable(packedvector-bench EXCLUDE_FROM_ALL packed_vector.cpp)
add_executable(route-bench        EXCLUDE_FROM_ALL route.cpp)
add_executable(rtree-bench        EXCLUDE_FROM_ALL static_rtree.cpp)

set (BENCHMARKS alias-bench bench json-render-bench match-bench packedvector-bench route-bench rtree-bench)

target_link_libraries(alias-bench        osrm_utils                            ${MAYBE_SHAPEFILE})
target_link_libraries(bench	             osrm_utils ENGINE ${ENGINE_LIBRARIES} ${MAYBE_SHAPEFILE})
target_link_libraries(json-render-bench	 osrm_utils ENGINE ${ENGINE_LIBRARIES} ${MAYBE_SHAPEFILE})
target_link_libraries(match-bench        osrm_utils ENGINE ${ENGINE_LIBRARIES} ${MAYBE_SHAPEFILE})
target_link_libraries(packedvector-bench osrm_utils                            ${MAYBE_SHAPEFILE})
target_link_libraries(route-bench        osrm_utils ENGINE ${ENGINE_LIBRARIES} ${MAYBE_SHAPEFILE})
target_link_libraries(rtree-bench        osrm_utils                            ${MAYBE_SHAPEFILE})

target_include_directories(rtree-bench PUBLIC ${PROJECT_SOURCE_DIR}/unit_tests)

add_custom_target(benchmarks DEPENDS ${BENCHMARKS})

### run the benchmarks ###

# fixture for test matrix

add_test(NAME fixture-berlin COMMAND make -C "${TEST_DATA_DIR}" monaco berlin)
set_tests_properties(fixture-berlin PROPERTIES FIXTURES_SETUP bench-fixtures-setup)

# test matrix

foreach(ALGORITHM ch mld)
  foreach(BENCHMARK match-bench route-bench)
    set (NAME "${BENCHMARK}-${ALGORITHM}")
    add_test(NAME "${NAME}" COMMAND ${BENCHMARK} "${MONACO}/${ALGORITHM}/monaco.osrm" ${ALGORITHM})
    set_tests_properties("${NAME}" PROPERTIES FIXTURES_REQUIRED bench-fixtures-setup LABELS benchmarks)
  endforeach()
endforeach()

add_test(NAME alias-bench COMMAND alias-bench)
add_test(NAME json-render-bench COMMAND json-render-bench
                "${TEST_DATA_DIR}/portugal_to_korea.json")
add_test(NAME packedvector-bench COMMAND packedvector-bench 10 100000)
add_test(NAME rtree-bench COMMAND rtree-bench
                "${MONACO}/ch/monaco.osrm.ramIndex"
                "${MONACO}/ch/monaco.osrm.fileIndex"
                "${MONACO}/ch/monaco.osrm.nbg_nodes")

set_tests_properties(alias-bench        PROPERTIES FIXTURES_REQUIRED bench-fixtures-setup LABELS benchmarks)
set_tests_properties(json-render-bench  PROPERTIES FIXTURES_REQUIRED bench-fixtures-setup LABELS benchmarks)
set_tests_properties(packedvector-bench PROPERTIES FIXTURES_REQUIRED bench-fixtures-setup LABELS benchmarks)
set_tests_properties(rtree-bench        PROPERTIES FIXTURES_REQUIRED bench-fixtures-setup LABELS benchmarks)

foreach(ALGORITHM ch mld)
    foreach(BENCHMARK nearest table trip route match)
        set (NAME "random-${BENCHMARK}-${ALGORITHM}")
        add_test(
            NAME "${NAME}"
            COMMAND
                "${CMAKE_CURRENT_BINARY_DIR}/bench"
                "${BERLIN}/${ALGORITHM}/berlin.osrm"
                "${ALGORITHM}"
                "${BENCHMARK}"
                5
                "${GPS_TRACES}")
        set_tests_properties("${NAME}" PROPERTIES FIXTURES_REQUIRED bench-fixtures-setup LABELS benchmarks)
        if(BUILD_NODE_PACKAGE)
            set (NAME "node-${BENCHMARK}-${ALGORITHM}")
            add_test(
                NAME "${NAME}"
                COMMAND
                    node "${PROJECT_SOURCE_DIR}/scripts/ci/bench.js"
                    "$ENV{OSRM_NODEJS_INSTALL_DIR}/lib/index.js"
                    "${BERLIN}/${ALGORITHM}/berlin.osrm"
                    "${ALGORITHM}"
                    "${BENCHMARK}"
                    5
                    "${GPS_TRACES}")
            set_tests_properties("${NAME}" PROPERTIES FIXTURES_REQUIRED bench-fixtures-setup LABELS benchmarks)
        endif()
    endforeach()
endforeach()
