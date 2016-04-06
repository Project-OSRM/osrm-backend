file(GLOB EngineTestsSources
    engine_tests.cpp
    engine/*.cpp)

file(GLOB ExtractorTestsSources
    extractor_tests.cpp
    extractor/*.cpp)

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


# FindPackage below overwrites Boost_LIBRARIES
set(AllBoostLibrariesExceptUnitTest ${Boost_LIBRARIES})

find_package(Boost 1.49.0 REQUIRED COMPONENTS unit_test_framework)

if(NOT WIN32)
  add_definitions(-DBOOST_TEST_DYN_LINK)
endif()

# After the find_package call we got only the unit test library
set(BoostUnitTestLibrary ${Boost_LIBRARIES})

include_directories(SYSTEM ${Boost_INCLUDE_DIRS})


target_include_directories(engine-tests PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(library-tests PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(util-tests PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})


target_link_libraries(engine-tests ${ENGINE_LIBRARIES} ${BoostUnitTestLibrary})
target_link_libraries(extractor-tests ${EXTRACTOR_LIBRARIES} ${BoostUnitTestLibrary})
target_link_libraries(library-tests osrm ${Boost_LIBRARIES} ${BoostUnitTestLibrary})
target_link_libraries(server-tests osrm ${Boost_LIBRARIES} ${BoostUnitTestLibrary})
target_link_libraries(util-tests ${UTIL_LIBRARIES} ${BoostUnitTestLibrary})


add_custom_target(tests
	DEPENDS
	engine-tests extractor-tests library-tests server-tests util-tests)
