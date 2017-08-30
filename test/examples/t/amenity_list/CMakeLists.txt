
add_test(NAME examples_amenity_list_node
         COMMAND osmium_amenity_list ${CMAKE_CURRENT_SOURCE_DIR}/node.osm)

set_tests_properties(examples_amenity_list_node PROPERTIES
                     PASS_REGULAR_EXPRESSION "  8\\.8721, 53\\.0966 post_office")

add_test(NAME examples_amenity_list_area
         COMMAND osmium_amenity_list ${CMAKE_CURRENT_SOURCE_DIR}/area.osm)

set_tests_properties(examples_amenity_list_area PROPERTIES
                     PASS_REGULAR_EXPRESSION "  8\\.5839, 53\\.5602 restaurant")

