file(GLOB RTreeBenchmarkSources
    *.cpp)

add_executable(rtree-bench
	EXCLUDE_FROM_ALL
	${RTreeBenchmarkSources}
	$<TARGET_OBJECTS:UTIL>)

target_include_directories(rtree-bench
	PUBLIC
	${PROJECT_SOURCE_DIR}/unit_tests)

target_link_libraries(rtree-bench
	${Boost_LIBRARIES}
	${CMAKE_THREAD_LIBS_INIT}
	${TBB_LIBRARIES})

add_custom_target(benchmarks
	DEPENDS
	rtree-bench)
