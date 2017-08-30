
add_test(NAME examples_tiles_zoom_too_large
         COMMAND osmium_tiles 50 1 1)

set_tests_properties(examples_tiles_zoom_too_large PROPERTIES WILL_FAIL true)

add_test(NAME examples_tiles_location_invalid
         COMMAND osmium_tiles 1 200 200)

set_tests_properties(examples_tiles_location_invalid PROPERTIES WILL_FAIL true)

add_test(NAME examples_tiles_okay
         COMMAND osmium_tiles 8 55.3 11.7)

set_tests_properties(examples_tiles_okay PROPERTIES
                     PASS_REGULAR_EXPRESSION "^WGS84:    lon=55.3 lat=11.7\nMercator: x=.*\nTile:     zoom=8 x=167 y=119\n$"
)

