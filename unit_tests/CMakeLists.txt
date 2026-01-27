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

add_executable(contractor-tests	       EXCLUDE_FROM_ALL ${ContractorTestsSources})
add_executable(customizer-tests	       EXCLUDE_FROM_ALL ${CustomizerTestsSources})
add_executable(engine-tests	           EXCLUDE_FROM_ALL	${EngineTestsSources})
add_executable(extractor-tests	       EXCLUDE_FROM_ALL	${ExtractorTestsSources})
add_executable(library-contract-tests  EXCLUDE_FROM_ALL	${LibraryContractTestsSources})
add_executable(library-customize-tests EXCLUDE_FROM_ALL ${LibraryCustomizeTestsSources})
add_executable(library-extract-tests   EXCLUDE_FROM_ALL	${LibraryExtractTestsSources})
add_executable(library-partition-tests EXCLUDE_FROM_ALL ${LibraryPartitionTestsSources})
add_executable(library-tests	       EXCLUDE_FROM_ALL	${LibraryTestsSources})
add_executable(partitioner-tests	   EXCLUDE_FROM_ALL	${PartitionTestsSources})
add_executable(server-tests	           EXCLUDE_FROM_ALL	${ServerTestsSources})
add_executable(storage-tests	       EXCLUDE_FROM_ALL ${StorageTestsSources})
add_executable(updater-tests	       EXCLUDE_FROM_ALL ${UpdaterTestsSources})
add_executable(util-tests	           EXCLUDE_FROM_ALL	${UtilTestsSources})

set(UPDATER_TEST_DATA_DIR "${CMAKE_SOURCE_DIR}/unit_tests/updater")
set(TEST_DATA_DIR "${CMAKE_SOURCE_DIR}/test/data")
add_dependencies(library-tests osrm-extract osrm-contract osrm-partition)

target_compile_definitions(contractor-tests        PRIVATE COMPILE_DEFINITIONS TEST_DATA_DIR="${TEST_DATA_DIR}")
target_compile_definitions(extractor-tests         PRIVATE COMPILE_DEFINITIONS OSRM_FIXTURES_DIR="${CMAKE_SOURCE_DIR}/unit_tests/fixtures")
target_compile_definitions(library-contract-tests  PRIVATE COMPILE_DEFINITIONS OSRM_TEST_DATA_DIR="${TEST_DATA_DIR}")
target_compile_definitions(library-customize-tests PRIVATE COMPILE_DEFINITIONS OSRM_TEST_DATA_DIR="${TEST_DATA_DIR}")
target_compile_definitions(library-extract-tests   PRIVATE COMPILE_DEFINITIONS OSRM_TEST_DATA_DIR="${TEST_DATA_DIR}")
target_compile_definitions(library-partition-tests PRIVATE COMPILE_DEFINITIONS OSRM_TEST_DATA_DIR="${TEST_DATA_DIR}")
target_compile_definitions(library-tests           PRIVATE COMPILE_DEFINITIONS OSRM_TEST_DATA_DIR="${TEST_DATA_DIR}")
target_compile_definitions(storage-tests           PRIVATE COMPILE_DEFINITIONS TEST_DATA_DIR="${TEST_DATA_DIR}")
target_compile_definitions(updater-tests           PRIVATE COMPILE_DEFINITIONS TEST_DATA_DIR="${UPDATER_TEST_DATA_DIR}")

target_include_directories(contractor-tests        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(customizer-tests        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(engine-tests            PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(library-contract-tests  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(library-customize-tests PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(library-extract-tests   PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(library-partition-tests PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(library-tests           PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(partitioner-tests       PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(storage-tests           PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(updater-tests           PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(util-tests              PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(contractor-tests        osrm_contract  ${BOOST_UNIT_TEST_LIBRARIES})
target_link_libraries(customizer-tests        osrm_customize ${BOOST_UNIT_TEST_LIBRARIES})
target_link_libraries(engine-tests            osrm_routed    ${BOOST_UNIT_TEST_LIBRARIES})
target_link_libraries(extractor-tests         osrm_extract   ${BOOST_UNIT_TEST_LIBRARIES})
target_link_libraries(library-contract-tests  osrm_contract  ${BOOST_UNIT_TEST_LIBRARIES})
target_link_libraries(library-customize-tests osrm_customize ${BOOST_UNIT_TEST_LIBRARIES})
target_link_libraries(library-extract-tests   osrm_extract   ${BOOST_UNIT_TEST_LIBRARIES})
target_link_libraries(library-partition-tests osrm_partition ${BOOST_UNIT_TEST_LIBRARIES})
target_link_libraries(library-tests           osrm_routed    ${BOOST_UNIT_TEST_LIBRARIES})
target_link_libraries(partitioner-tests       osrm_partition ${BOOST_UNIT_TEST_LIBRARIES})
target_link_libraries(server-tests            osrm_routed    ${BOOST_UNIT_TEST_LIBRARIES} $<TARGET_OBJECTS:SERVER>)
target_link_libraries(storage-tests           osrm_store     ${BOOST_UNIT_TEST_LIBRARIES})
target_link_libraries(updater-tests           osrm_update    ${BOOST_UNIT_TEST_LIBRARIES})
target_link_libraries(util-tests              osrm_utils     ${BOOST_UNIT_TEST_LIBRARIES})

add_custom_target(tests
  DEPENDS
    contractor-tests
    customizer-tests
    engine-tests
    extractor-tests
    library-contract-tests
    library-customize-tests
    library-extract-tests
    library-partition-tests
    partitioner-tests
    server-tests
    storage-tests
    updater-tests
    util-tests
)

if (NOT MSVC)
  # FIXME: MSVC goes out of mem on github ci compiling target library-tests
  add_dependencies(tests library-tests)
endif()

post_build_dir("OSRM_UNIT_TESTS_BUILD_DIR")
