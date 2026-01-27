add_executable(alias-bench        EXCLUDE_FROM_ALL alias.cpp)
add_executable(bench	          EXCLUDE_FROM_ALL bench.cpp)
add_executable(json-render-bench  EXCLUDE_FROM_ALL json_render.cpp)
add_executable(match-bench        EXCLUDE_FROM_ALL match.cpp)
add_executable(packedvector-bench EXCLUDE_FROM_ALL packed_vector.cpp)
add_executable(route-bench        EXCLUDE_FROM_ALL route.cpp)
add_executable(rtree-bench        EXCLUDE_FROM_ALL static_rtree.cpp)

target_link_libraries(alias-bench        osrm_utils  ${MAYBE_SHAPEFILE})
target_link_libraries(bench	             osrm_routed ${MAYBE_SHAPEFILE})
target_link_libraries(json-render-bench	 osrm_routed ${MAYBE_SHAPEFILE})
target_link_libraries(match-bench        osrm_routed ${MAYBE_SHAPEFILE})
target_link_libraries(packedvector-bench osrm_utils  ${MAYBE_SHAPEFILE})
target_link_libraries(route-bench        osrm_routed ${MAYBE_SHAPEFILE})
target_link_libraries(rtree-bench        osrm_utils  ${MAYBE_SHAPEFILE})

target_include_directories(rtree-bench PUBLIC ${PROJECT_SOURCE_DIR}/unit_tests)

add_custom_target(benchmarks
  DEPENDS
    rtree-bench
    packedvector-bench
    match-bench
    route-bench
    bench
    json-render-bench
    alias-bench)

post_build_dir("OSRM_BENCHMARKS_BUILD_DIR")
