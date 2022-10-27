file(GLOB EngineTestsSources
    engine_tests.cpp
    engine/*.cpp)

file(GLOB ContractorTestsSources
    contractor_tests.cpp
    contractor/*.cpp)

file(GLOB ExtractorTestsSources
    extractor_tests.cpp
    extractor/*.cpp)

file(GLOB PartitionTestsSources
    partitioner_tests.cpp
    partitioner/*.cpp)

file(GLOB CustomizerTestsSources
    customizer_tests.cpp
    customizer/*.cpp)

file(GLOB UpdaterTestsSources
    updater_tests.cpp
    updater/*.cpp)

file(GLOB StorageTestsSources
    storage_tests.cpp
    storage/*.cpp)

file(GLOB LibraryTestsSources
    library_tests.cpp
    library/*.cpp)
list(REMOVE_ITEM LibraryTestsSources
    ${CMAKE_CURRENT_SOURCE_DIR}/library/contract.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/library/customize.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/library/extract.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/library/partition.cpp)

file(GLOB LibraryExtractTestsSources
    library_tests.cpp
    library/extract.cpp)

file(GLOB LibraryContractTestsSources
    library_tests.cpp
    library/contract.cpp)

file(GLOB LibraryCustomizeTestsSources
    library_tests.cpp
    library/customize.cpp)

file(GLOB LibraryPartitionTestsSources
    library_tests.cpp
    library/partition.cpp)

file(GLOB ServerTestsSources
    server_tests.cpp
    server/*.cpp)

file(GLOB UtilTestsSources
    util_tests.cpp
    util/*.cpp)

add_executable(engine-tests
	EXCLUDE_FROM_ALL
	${EngineTestsSources}
        $<TARGET_OBJECTS:ENGINE> $<TARGET_OBJECTS:STORAGE> $<TARGET_OBJECTS:UTIL> $<TARGET_OBJECTS:MICROTAR>)

add_executable(contractor-tests
	EXCLUDE_FROM_ALL
    ${ContractorTestsSources}
    $<TARGET_OBJECTS:CONTRACTOR> $<TARGET_OBJECTS:UPDATER> $<TARGET_OBJECTS:UTIL> $<TARGET_OBJECTS:MICROTAR>)

add_executable(extractor-tests
	EXCLUDE_FROM_ALL
	${ExtractorTestsSources}
	$<TARGET_OBJECTS:EXTRACTOR> $<TARGET_OBJECTS:UTIL>)

add_executable(partitioner-tests
	EXCLUDE_FROM_ALL
	${PartitionTestsSources}
	$<TARGET_OBJECTS:PARTITIONER> $<TARGET_OBJECTS:UTIL>)

add_executable(customizer-tests
	EXCLUDE_FROM_ALL
    ${CustomizerTestsSources}
    $<TARGET_OBJECTS:CUSTOMIZER> $<TARGET_OBJECTS:UPDATER> $<TARGET_OBJECTS:UTIL>)

add_executable(updater-tests
	EXCLUDE_FROM_ALL
    ${UpdaterTestsSources}
    $<TARGET_OBJECTS:UPDATER> $<TARGET_OBJECTS:UPDATER> $<TARGET_OBJECTS:UTIL>)

add_executable(storage-tests
	EXCLUDE_FROM_ALL
    ${StorageTestsSources}
    $<TARGET_OBJECTS:STORAGE> $<TARGET_OBJECTS:UTIL>)

add_executable(library-tests
	EXCLUDE_FROM_ALL
	${LibraryTestsSources})

add_executable(library-extract-tests
	EXCLUDE_FROM_ALL
	${LibraryExtractTestsSources})

add_executable(library-contract-tests
	EXCLUDE_FROM_ALL
	${LibraryContractTestsSources})

add_executable(library-customize-tests
    EXCLUDE_FROM_ALL
    ${LibraryCustomizeTestsSources})

add_executable(library-partition-tests
    EXCLUDE_FROM_ALL
    ${LibraryPartitionTestsSources})

if (BUILD_ROUTED)
  add_executable(server-tests
	EXCLUDE_FROM_ALL
	${ServerTestsSources}
	$<TARGET_OBJECTS:UTIL> $<TARGET_OBJECTS:SERVER>)
endif()

add_executable(util-tests
	EXCLUDE_FROM_ALL
	${UtilTestsSources}
        $<TARGET_OBJECTS:UTIL> $<TARGET_OBJECTS:MICROTAR>)


if(NOT WIN32 AND NOT Boost_USE_STATIC_LIBS)
  add_definitions(-DBOOST_TEST_DYN_LINK)
endif()

set(UPDATER_TEST_DATA_DIR "${CMAKE_SOURCE_DIR}/unit_tests/updater")
set(TEST_DATA_DIR "${CMAKE_SOURCE_DIR}/test/data")
add_dependencies(library-tests osrm-extract osrm-contract osrm-partition)
# We can't run this Makefile on windows
if (NOT WIN32)
  add_custom_command(TARGET library-tests POST_BUILD COMMAND make -C ${TEST_DATA_DIR} OSRM_BUILD_DIR=$ENV{OSRM_BUILD_DIR})
endif()

target_compile_definitions(extractor-tests PRIVATE COMPILE_DEFINITIONS OSRM_FIXTURES_DIR="${CMAKE_SOURCE_DIR}/unit_tests/fixtures")
target_compile_definitions(library-tests PRIVATE COMPILE_DEFINITIONS OSRM_TEST_DATA_DIR="${TEST_DATA_DIR}")
target_compile_definitions(library-extract-tests PRIVATE COMPILE_DEFINITIONS OSRM_TEST_DATA_DIR="${TEST_DATA_DIR}")
target_compile_definitions(library-contract-tests PRIVATE COMPILE_DEFINITIONS OSRM_TEST_DATA_DIR="${TEST_DATA_DIR}")
target_compile_definitions(library-customize-tests PRIVATE COMPILE_DEFINITIONS OSRM_TEST_DATA_DIR="${TEST_DATA_DIR}")
target_compile_definitions(library-partition-tests PRIVATE COMPILE_DEFINITIONS OSRM_TEST_DATA_DIR="${TEST_DATA_DIR}")
target_compile_definitions(updater-tests PRIVATE COMPILE_DEFINITIONS TEST_DATA_DIR="${UPDATER_TEST_DATA_DIR}")
target_compile_definitions(contractor-tests PRIVATE COMPILE_DEFINITIONS TEST_DATA_DIR="${TEST_DATA_DIR}")
target_compile_definitions(storage-tests PRIVATE COMPILE_DEFINITIONS TEST_DATA_DIR="${TEST_DATA_DIR}")

target_include_directories(engine-tests PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(library-tests PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(library-extract-tests PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(library-contract-tests PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(library-customize-tests PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(library-partition-tests PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(util-tests PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(partitioner-tests PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(customizer-tests PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(updater-tests PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(contractor-tests PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(storage-tests PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(engine-tests ${ENGINE_LIBRARIES} ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})
target_link_libraries(extractor-tests osrm_extract osrm_guidance ${EXTRACTOR_LIBRARIES} ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})
target_link_libraries(partitioner-tests osrm_partition ${PARTITIONER_LIBRARIES} ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})
target_link_libraries(customizer-tests osrm_customize ${CUSTOMIZER_LIBRARIES} ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})
target_link_libraries(updater-tests osrm_update ${UPDATER_LIBRARIES} ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})
target_link_libraries(library-tests osrm ${ENGINE_LIBRARIES} ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})
target_link_libraries(library-extract-tests osrm_extract osrm_guidance ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})
target_link_libraries(library-contract-tests osrm_contract ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})
target_link_libraries(library-customize-tests osrm_customize ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})
target_link_libraries(library-partition-tests osrm_partition ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})
if (BUILD_ROUTED)
  target_link_libraries(server-tests osrm ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})
endif()
target_link_libraries(util-tests ${UTIL_LIBRARIES} ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})
target_link_libraries(contractor-tests osrm_contract ${CONTRACTOR_LIBRARIES} ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})
target_link_libraries(storage-tests osrm_store ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})

add_custom_target(tests
	DEPENDS engine-tests extractor-tests contractor-tests partitioner-tests updater-tests customizer-tests library-tests library-extract-tests library-contract-tests library-customize-tests library-partition-tests server-tests util-tests storage-tests)
