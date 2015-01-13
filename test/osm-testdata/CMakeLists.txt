message(STATUS "Configuring osm-testdata tests...")

if(NOT GDAL_FOUND OR
   NOT EXPAT_FOUND)
    message("Sorry, building osm-testdata tests needs GDAL and Expat")
    return()
endif()

set(DART_TESTING_TIMEOUT 300)
include(Dart)

include_directories("include")
include_directories("../include")

# In 'Dev' mode: compile with very strict warnings and turn them into errors.
if(CMAKE_BUILD_TYPE STREQUAL "Dev")
    add_definitions(-Werror ${OSMIUM_WARNING_OPTIONS} -Wno-unused-variable)
endif()

get_filename_component(TESTDATA_DIR ../../../osm-testdata ABSOLUTE)
if(NOT EXISTS ${TESTDATA_DIR})
    execute_process(COMMAND git clone https://github.com/osmcode/osm-testdata.git ${TESTDATA_DIR})
endif()

#----------------------------------------------------------------------

file(GLOB TESTCASE_CPPS testcases/*.cpp)
add_executable(testdata-testcases testdata-testcases.cpp ${TESTCASE_CPPS})
target_link_libraries(testdata-testcases
                      ${OSMIUM_XML_LIBRARIES}
)
add_test(NAME testdata-testcases
         COMMAND testdata-testcases
)
set_tests_properties(testdata-testcases
    PROPERTIES ENVIRONMENT "TESTCASES_DIR=${TESTDATA_DIR}/grid/data")

#----------------------------------------------------------------------

add_executable(testdata-overview testdata-overview.cpp)
target_link_libraries(testdata-overview
                      ${OSMIUM_XML_LIBRARIES}
                      ${GDAL_LIBRARIES}
)
add_test(NAME testdata-overview
         COMMAND testdata-overview ${TESTDATA_DIR}/grid/data/all.osm
)

#----------------------------------------------------------------------

add_executable(testdata-xml testdata-xml.cpp)
target_link_libraries(testdata-xml
                      ${OSMIUM_XML_LIBRARIES}
)
add_test(NAME testdata-xml
         COMMAND testdata-xml
)
set_tests_properties(testdata-xml
    PROPERTIES ENVIRONMENT "TESTDIR=${TESTDATA_DIR}/xml/data")

#----------------------------------------------------------------------

add_executable(testdata-multipolygon testdata-multipolygon.cpp)
target_link_libraries(testdata-multipolygon
                      ${OSMIUM_XML_LIBRARIES}
                      ${GDAL_LIBRARIES}
)

if(WIN32)
    set(MULTIPOLYGON_TEST_SCRIPT "run-testdata-multipolygon.bat")
else()
    set(MULTIPOLYGON_TEST_SCRIPT "run-testdata-multipolygon.sh")
endif()

if(MSVC)
    set(EXE_DIR ${CMAKE_BUILD_TYPE})
else()
    set(EXE_DIR .)
endif()

add_test(NAME testdata-multipolygon
         COMMAND ${CMAKE_SOURCE_DIR}/test/osm-testdata/${MULTIPOLYGON_TEST_SCRIPT}
         ${TESTDATA_DIR}
         ${EXE_DIR}
)

#----------------------------------------------------------------------
