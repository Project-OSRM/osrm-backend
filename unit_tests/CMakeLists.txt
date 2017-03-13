file(GLOB EngineTestsSources
    engine_tests.cpp
    engine/*.cpp)

file(GLOB ExtractorTestsSources
    extractor_tests.cpp
    extractor/*.cpp)

file(GLOB PartitionTestsSources
    partition_tests.cpp
    partition/*.cpp)

file(GLOB CustomizerTestsSources
    customizer_tests.cpp
    customizer/*.cpp)

file(GLOB LibraryTestsSources
    library_tests.cpp
    library/*.cpp)
list(REMOVE_ITEM LibraryTestsSources ${CMAKE_CURRENT_SOURCE_DIR}/library/extract.cpp ${CMAKE_CURRENT_SOURCE_DIR}/library/contract.cpp)

file(GLOB LibraryExtractTestsSources
    library_tests.cpp
    library/extract.cpp)

file(GLOB LibraryContractTestsSources
    library_tests.cpp
    library/contract.cpp)

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

add_executable(customizer-tests
	EXCLUDE_FROM_ALL
    ${CustomizerTestsSources}
    $<TARGET_OBJECTS:CUSTOMIZER> $<TARGET_OBJECTS:UPDATER> $<TARGET_OBJECTS:UTIL>)

add_executable(library-tests
	EXCLUDE_FROM_ALL
	${LibraryTestsSources})

add_executable(library-extract-tests
	EXCLUDE_FROM_ALL
	${LibraryExtractTestsSources})

add_executable(library-contract-tests
	EXCLUDE_FROM_ALL
	${LibraryContractTestsSources})

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

set(TEST_DATA_DIR "${CMAKE_SOURCE_DIR}/test/data")
add_dependencies(library-tests osrm-extract osrm-contract osrm-partition)
# We can't run this Makefile on windows
if (NOT WIN32)
  add_custom_command(TARGET library-tests POST_BUILD COMMAND make -C ${TEST_DATA_DIR})
endif()

target_compile_definitions(extractor-tests PRIVATE COMPILE_DEFINITIONS OSRM_FIXTURES_DIR="${CMAKE_SOURCE_DIR}/unit_tests/fixtures")
target_compile_definitions(library-tests PRIVATE COMPILE_DEFINITIONS OSRM_TEST_DATA_DIR="${TEST_DATA_DIR}")
target_compile_definitions(library-extract-tests PRIVATE COMPILE_DEFINITIONS OSRM_TEST_DATA_DIR="${TEST_DATA_DIR}")
target_compile_definitions(library-contract-tests PRIVATE COMPILE_DEFINITIONS OSRM_TEST_DATA_DIR="${TEST_DATA_DIR}")

target_include_directories(engine-tests PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(library-tests PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(library-extract-tests PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(library-contract-tests PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(util-tests PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(partition-tests PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(customizer-tests PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(engine-tests ${ENGINE_LIBRARIES} ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})
target_link_libraries(extractor-tests ${EXTRACTOR_LIBRARIES} ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})
target_link_libraries(partition-tests ${PARTITIONER_LIBRARIES} ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})
target_link_libraries(customizer-tests ${CUSTOMIZER_LIBRARIES} ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})
target_link_libraries(library-tests osrm ${ENGINE_LIBRARIES} ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})
target_link_libraries(library-extract-tests osrm_extract ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})
target_link_libraries(library-contract-tests osrm_contract ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})
target_link_libraries(server-tests osrm ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})
target_link_libraries(util-tests ${UTIL_LIBRARIES} ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})

add_custom_target(tests
	DEPENDS engine-tests extractor-tests partition-tests customizer-tests library-tests library-extract-tests server-tests util-tests)
