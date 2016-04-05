file(GLOB ExtractorTest_SOURCES
	extractor_tests.cpp
	extractor/*.cpp)

file(GLOB EngineTest_SOURCES
	engine_tests.cpp
	engine/*.cpp)

file(GLOB UtilTest_SOURCES
	util_tests.cpp
	util/*)


add_executable(extractor-tests
	EXCLUDE_FROM_ALL
	${ExtractorTest_SOURCES}
	$<TARGET_OBJECTS:EXTRACTOR>
	$<TARGET_OBJECTS:UTIL>)

add_executable(engine-tests
	EXCLUDE_FROM_ALL
	${EngineTest_SOURCES}
	$<TARGET_OBJECTS:EXTRACTOR>
	$<TARGET_OBJECTS:UTIL>)

add_executable(util-tests
	EXCLUDE_FROM_ALL
	${UtilTest_SOURCES}
	$<TARGET_OBJECTS:UTIL>)


# FindPackage below overwrites Boost_LIBRARIES
set(AllBoostLibrariesExceptUnitTest ${Boost_LIBRARIES})

find_package(Boost 1.49.0 REQUIRED COMPONENTS unit_test_framework)

if(NOT WIN32)
  add_definitions(-DBOOST_TEST_DYN_LINK)
endif()

include_directories(SYSTEM ${Boost_INCLUDE_DIRS})


target_include_directories(extractor-tests PUBLIC ${Boost_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(engine-tests PUBLIC ${Boost_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(util-tests PUBLIC ${Boost_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR})


# Strictly speaking we only need to link against a subset of all osrm libs, but it seems like the tests depend not strictly on their lib counterpart (e.g. extractor test not only depends on EXTRACTOR_LIBRARIES) --- this should be fixed in the future.
set(OSRMLibraries osrm osrm_contract osrm_extract osrm_store)

target_link_libraries(extractor-tests ${OSRMLibraries} ${AllBoostLibrariesExceptUnitTest} ${Boost_LIBRARIES})
target_link_libraries(engine-tests ${OSRMLibraries} ${AllBoostLibrariesExceptUnitTest} ${Boost_LIBRARIES})
target_link_libraries(util-tests ${OSRMLibraries} ${AllBoostLibrariesExceptUnitTest} ${Boost_LIBRARIES})


add_custom_target(tests
	DEPENDS
	engine-tests
	extractor-tests
	util-tests)
