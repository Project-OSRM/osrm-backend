
add_test(NAME examples_amenity_list
         COMMAND osmium_amenity_list ${CMAKE_CURRENT_SOURCE_DIR}/node.osm)

set_tests_properties(examples_amenity_list PROPERTIES
                     PASS_REGULAR_EXPRESSION "  8\\.8721, 53\\.0966 post_office")

