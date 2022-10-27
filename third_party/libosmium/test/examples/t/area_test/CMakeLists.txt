
add_test(NAME examples_area_test_help
         COMMAND osmium_area_test -h)

set_tests_properties(examples_area_test_help PROPERTIES
                     PASS_REGULAR_EXPRESSION "^osmium_area_test .* OSMFILE")

add_test(NAME examples_area_test_data
         COMMAND osmium_area_test -o ${CMAKE_CURRENT_SOURCE_DIR}/data.osm)

set_tests_properties(examples_area_test_data PROPERTIES
                     PASS_REGULAR_EXPRESSION "\nWarning! Some member ways missing for these multipolygon relations: 701901\n$")

add_test(NAME examples_area_test_dump
         COMMAND osmium_area_test -o ${CMAKE_CURRENT_SOURCE_DIR}/data.osm)

set_tests_properties(examples_area_test_dump PROPERTIES
                     PASS_REGULAR_EXPRESSION "  id=1403801")

add_test(NAME examples_area_test_wkt
         COMMAND osmium_area_test -w ${CMAKE_CURRENT_SOURCE_DIR}/data.osm)

set_tests_properties(examples_area_test_wkt PROPERTIES
                     PASS_REGULAR_EXPRESSION "MULTIPOLYGON\\(\\(\\(7.11 1.01,7.14 1.01,7.14 1.04,7.11 1.04,7.11 1.01\\)\\)\\)\n")

