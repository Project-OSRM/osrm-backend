#-----------------------------------------------------------------------------
#
#  CMake config
#
#  vtzero examples
#
#-----------------------------------------------------------------------------

include_directories(SYSTEM "${CMAKE_SOURCE_DIR}/include-external")

set(TEST_FILE "${CMAKE_SOURCE_DIR}/test/data/mapbox-streets-v6-14-8714-8017.mvt")

add_executable(vtzero-check vtzero-check.cpp utils.cpp)

add_executable(vtzero-create vtzero-create.cpp utils.cpp)

add_executable(vtzero-encode-geom vtzero-encode-geom.cpp utils.cpp)

add_executable(vtzero-stats vtzero-stats.cpp utils.cpp)

add_executable(vtzero-streets vtzero-streets.cpp utils.cpp)

#-------------------------------------------------------------

add_executable(vtzero-filter vtzero-filter.cpp utils.cpp)

add_test(NAME vtzero-filter-empty
            COMMAND vtzero-filter)
set_tests_properties(vtzero-filter-empty PROPERTIES
                        PASS_REGULAR_EXPRESSION "^Error in command line: Missing file name of vector tile to read")

add_test(NAME vtzero-filter-help
            COMMAND vtzero-filter -h)
set_tests_properties(vtzero-filter-help PROPERTIES
                        PASS_REGULAR_EXPRESSION "^usage:\n  vtzero-filter")

add_test(NAME vtzero-filter-layer
            COMMAND vtzero-filter -o ${CMAKE_CURRENT_BINARY_DIR}/bridges.mvt ${TEST_FILE} bridge)

add_test(NAME vtzero-filter-feature
            COMMAND vtzero-filter -o ${CMAKE_CURRENT_BINARY_DIR}/bridges.mvt ${TEST_FILE} waterway_label 221925711)

add_test(NAME vtzero-filter-invalid-id
            COMMAND vtzero-filter -o ${CMAKE_CURRENT_BINARY_DIR}/bridges.mvt ${TEST_FILE} waterway_label abc)
set_tests_properties(vtzero-filter-invalid-id PROPERTIES
                        WILL_FAIL true)

#-------------------------------------------------------------

add_executable(vtzero-show vtzero-show.cpp utils.cpp)

add_test(NAME vtzero-show-empty
            COMMAND vtzero-show)
set_tests_properties(vtzero-show-empty PROPERTIES
                        PASS_REGULAR_EXPRESSION "^Error in command line: Missing file name of vector tile to read")

add_test(NAME vtzero-show-help
            COMMAND vtzero-show -h)
set_tests_properties(vtzero-show-help PROPERTIES
                        PASS_REGULAR_EXPRESSION "^usage:\n  vtzero-show")

add_test(NAME vtzero-show-layers
            COMMAND vtzero-show -l ${TEST_FILE})
set_tests_properties(vtzero-show-layers PROPERTIES
                        PASS_REGULAR_EXPRESSION "^landuse 78\nwaterway 327\n.*\nwaterway_label 4\n$")

add_test(NAME vtzero-show-layer-num
            COMMAND vtzero-show ${TEST_FILE} 2)
set_tests_properties(vtzero-show-layer-num PROPERTIES
                        PASS_REGULAR_EXPRESSION "layer: [0-9]+\n  name: water\n")

add_test(NAME vtzero-show-layer-name
            COMMAND vtzero-show ${CMAKE_SOURCE_DIR}/test/data/mapbox-streets-v6-14-8714-8017.mvt water)
set_tests_properties(vtzero-show-layer-name PROPERTIES
                        PASS_REGULAR_EXPRESSION "layer: [0-9]+\n  name: water\n")

#-------------------------------------------------------------

file(GLOB ext_tests RELATIVE ${CMAKE_SOURCE_DIR}/test/data/ ${CMAKE_SOURCE_DIR}/test/data/*.mvt)

foreach(_test IN LISTS ext_tests)
    message(STATUS "Adding ext test: ${_test}")
    add_test(NAME ext-tests-${_test}
             COMMAND vtzero-show ${CMAKE_SOURCE_DIR}/test/data/${_test})
endforeach()


#-----------------------------------------------------------------------------
