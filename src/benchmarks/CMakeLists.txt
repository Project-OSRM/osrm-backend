file(GLOB RTreeBenchmarkSources static_rtree.cpp)
file(GLOB MatchBenchmarkSources match.cpp)
file(GLOB AliasBenchmarkSources alias.cpp)
file(GLOB PackedVectorBenchmarkSources packed_vector.cpp)

add_executable(rtree-bench
	EXCLUDE_FROM_ALL
	${RTreeBenchmarkSources}
    $<TARGET_OBJECTS:MICROTAR> $<TARGET_OBJECTS:UTIL>)

target_include_directories(rtree-bench
	PUBLIC
	${PROJECT_SOURCE_DIR}/unit_tests)

target_link_libraries(rtree-bench
	${BOOST_BASE_LIBRARIES}
	${CMAKE_THREAD_LIBS_INIT}
	${TBB_LIBRARIES}
	${MAYBE_SHAPEFILE})

add_executable(match-bench
	EXCLUDE_FROM_ALL
	${MatchBenchmarkSources}
	$<TARGET_OBJECTS:UTIL>)

target_link_libraries(match-bench
	osrm
	${BOOST_BASE_LIBRARIES}
	${CMAKE_THREAD_LIBS_INIT}
	${TBB_LIBRARIES}
	${MAYBE_SHAPEFILE})

add_executable(alias-bench
	EXCLUDE_FROM_ALL
    ${AliasBenchmarkSources}
	$<TARGET_OBJECTS:UTIL>)

target_link_libraries(alias-bench
	${BOOST_BASE_LIBRARIES}
	${CMAKE_THREAD_LIBS_INIT}
	${TBB_LIBRARIES}
	${MAYBE_SHAPEFILE})

add_executable(packedvector-bench
	EXCLUDE_FROM_ALL
    ${PackedVectorBenchmarkSources}
	$<TARGET_OBJECTS:UTIL>)

target_link_libraries(packedvector-bench
	${BOOST_BASE_LIBRARIES}
	${CMAKE_THREAD_LIBS_INIT}
	${TBB_LIBRARIES}
    ${MAYBE_SHAPEFILE})


add_custom_target(benchmarks
	DEPENDS
	rtree-bench
	packedvector-bench
	match-bench
    alias-bench)
