
add_test(NAME examples_road_length
         COMMAND osmium_road_length ${CMAKE_CURRENT_SOURCE_DIR}/road.osm)

set_tests_properties(examples_road_length PROPERTIES
                     PASS_REGULAR_EXPRESSION "^Length: 0\\.405.*km\n$"
)

