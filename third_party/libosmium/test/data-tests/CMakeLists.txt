#-----------------------------------------------------------------------------
#
#  CMake Config
#
#  Libosmium data tests
#
#-----------------------------------------------------------------------------

message(STATUS "Configuring data tests")

if(NOT GDAL_FOUND OR NOT EXPAT_FOUND)
    message(STATUS "Sorry, building data tests needs GDAL and Expat")
    message(STATUS "Configuring data tests - failed")
    return()
endif()

message(STATUS "Looking for osm-testdata")
find_path(OSM_TESTDATA grid/data/all.osm HINT osm-testdata)
if(OSM_TESTDATA STREQUAL "OSM_TESTDATA-NOTFOUND")
    message(STATUS "Looking for osm-testdata - not found (data tests disabled)")
    message(STATUS "Configuring data tests - failed")
    return()
endif()
message(STATUS "Looking for osm-testdata - found")


#-----------------------------------------------------------------------------

include_directories(SYSTEM ../catch)
include_directories(include)
include_directories(../include)


#-----------------------------------------------------------------------------
#
#  testcases
#
#-----------------------------------------------------------------------------
file(GLOB TESTCASE_CPPS testcases/*.cpp)
add_executable(testdata-testcases testdata-testcases.cpp ${TESTCASE_CPPS})
set_pthread_on_target(testdata-testcases)
target_link_libraries(testdata-testcases
                      ${OSMIUM_XML_LIBRARIES}
)
add_test(NAME testdata-testcases
         COMMAND testdata-testcases
)
set_tests_properties(testdata-testcases PROPERTIES
    ENVIRONMENT "TESTCASES_DIR=${OSM_TESTDATA}/grid/data"
    LABELS "data;fast")


#-----------------------------------------------------------------------------
#
#  xml
#
#-----------------------------------------------------------------------------
add_executable(testdata-xml testdata-xml.cpp)
set_pthread_on_target(testdata-xml)
target_link_libraries(testdata-xml
                      ${OSMIUM_XML_LIBRARIES}
)
add_test(NAME testdata-xml
         COMMAND testdata-xml
)
set_tests_properties(testdata-xml PROPERTIES
    ENVIRONMENT "TESTDIR=${OSM_TESTDATA}/xml/data"
    LABELS "data;fast")


#-----------------------------------------------------------------------------
#
#  overview
#
#-----------------------------------------------------------------------------
add_executable(testdata-overview testdata-overview.cpp)
set_pthread_on_target(testdata-overview)
target_link_libraries(testdata-overview
                      ${OSMIUM_XML_LIBRARIES}
                      ${GDAL_LIBRARIES}
)
add_test(NAME testdata-overview
         COMMAND testdata-overview ${OSM_TESTDATA}/grid/data/all.osm
)
set_tests_properties(testdata-overview PROPERTIES
    LABELS "data;slow")


#-----------------------------------------------------------------------------
#
#  multipolygon
#
#-----------------------------------------------------------------------------

find_program(RUBY ruby)
if(RUBY)
    find_package(Gem COMPONENTS json)
endif()

find_program(SPATIALITE spatialite)

if(RUBY AND GEM_json_FOUND AND SPATIALITE)
    add_executable(testdata-multipolygon testdata-multipolygon.cpp)
    set_pthread_on_target(testdata-multipolygon)
    target_link_libraries(testdata-multipolygon
                          ${OSMIUM_XML_LIBRARIES}
                          ${GDAL_LIBRARIES}
    )

    add_test(NAME testdata-multipolygon
             COMMAND ${CMAKE_COMMAND}
                 -D OSM_TESTDATA=${OSM_TESTDATA}
                 -D RUBY=${RUBY}
                 -D EXECUTABLE=$<TARGET_FILE:testdata-multipolygon>
                 -P ${CMAKE_CURRENT_SOURCE_DIR}/run-testdata-multipolygon.cmake)

    set_tests_properties(testdata-multipolygon PROPERTIES LABELS "data;slow")
    configure_file(multipolygon.qgs ${CMAKE_CURRENT_BINARY_DIR}/multipolygon.qgs @ONLY)
else()
    message(WARNING "Disabled testdata-multipolygon test because 'ruby' and/or 'json' ruby gem and/or 'spatialite' was not found")
endif()


#-----------------------------------------------------------------------------
message(STATUS "Configuring data tests - done")


#-----------------------------------------------------------------------------
