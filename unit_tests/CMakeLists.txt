file(GLOB EngineTestsSources
    engine_tests.cpp
    engine/*.cpp)

file(GLOB ExtractorTestsSources
    extractor_tests.cpp
    extractor/*.cpp)

file(GLOB PartitionTestsSources
    partition_tests.cpp
    partition/*.cpp)

file(GLOB LibraryTestsSources
    library_tests.cpp
    library/*.cpp)

file(GLOB ServerTestsSources
    server_tests.cpp
    server/*.cpp)

file(GLOB UtilTestsSources
    util_tests.cpp
    util/*.cpp)


add_executable(engine-tests
	EXCLUDE_FROM_ALL
	${EngineTestsSources}
	$<TARGET_OBJECTS:ENGINE> $<TARGET_OBJECTS:STORAGE> $<TARGET_OBJECTS:UTIL>)

add_executable(extractor-tests
	EXCLUDE_FROM_ALL
	${ExtractorTestsSources}
	$<TARGET_OBJECTS:EXTRACTOR> $<TARGET_OBJECTS:UTIL>)

add_executable(partition-tests
	EXCLUDE_FROM_ALL
	${PartitionTestsSources}
	$<TARGET_OBJECTS:PARTITIONER> $<TARGET_OBJECTS:UTIL>)

add_executable(library-tests
	EXCLUDE_FROM_ALL
	${LibraryTestsSources})

add_executable(server-tests
	EXCLUDE_FROM_ALL
	${ServerTestsSources}
	$<TARGET_OBJECTS:UTIL> $<TARGET_OBJECTS:SERVER>)

add_executable(util-tests
	EXCLUDE_FROM_ALL
	${UtilTestsSources}
	$<TARGET_OBJECTS:UTIL>)


if(NOT WIN32 AND NOT Boost_USE_STATIC_LIBS)
  add_definitions(-DBOOST_TEST_DYN_LINK)
endif()

target_compile_definitions(extractor-tests PRIVATE COMPILE_DEFINITIONS OSRM_FIXTURES_DIR="${CMAKE_SOURCE_DIR}/unit_tests/fixtures")

target_include_directories(engine-tests PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(library-tests PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(util-tests PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})


target_link_libraries(engine-tests ${ENGINE_LIBRARIES} ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})
target_link_libraries(extractor-tests ${EXTRACTOR_LIBRARIES} ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})
target_link_libraries(partition-tests ${PARTITIONER_LIBRARIES} ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})
target_link_libraries(library-tests osrm ${ENGINE_LIBRARIES} ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})
target_link_libraries(server-tests osrm ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})
target_link_libraries(util-tests ${UTIL_LIBRARIES} ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})


add_custom_target(tests
	DEPENDS
	engine-tests extractor-tests partition-tests library-tests server-tests util-tests)
