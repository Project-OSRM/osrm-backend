file(GLOB RTreeBenchmarkSources static_rtree.cpp)
file(GLOB MatchBenchmarkSources match.cpp)
file(GLOB AliasBenchmarkSources alias.cpp)
file(GLOB RouteBenchmarkSources route.cpp)
file(GLOB TableBenchmarkSources table.cpp)

add_executable(rtree-bench
	EXCLUDE_FROM_ALL
	${RTreeBenchmarkSources}
	$<TARGET_OBJECTS:UTIL>)

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

add_executable(route-bench
	EXCLUDE_FROM_ALL
	${RouteBenchmarkSources}
	$<TARGET_OBJECTS:UTIL>)

target_link_libraries(route-bench
	osrm
	${BOOST_BASE_LIBRARIES}
	${CMAKE_THREAD_LIBS_INIT}
	${TBB_LIBRARIES})

add_executable(table-bench
	EXCLUDE_FROM_ALL
	${TableBenchmarkSources}
	$<TARGET_OBJECTS:UTIL>)

target_link_libraries(table-bench
	osrm
	${BOOST_BASE_LIBRARIES}
	${CMAKE_THREAD_LIBS_INIT}
	${TBB_LIBRARIES})

add_custom_target(benchmarks
	DEPENDS
	rtree-bench
	match-bench
    alias-bench
	route-bench
	table-bench)
