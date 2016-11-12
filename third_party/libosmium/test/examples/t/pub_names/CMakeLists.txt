
add_test(NAME examples_pub_names
         COMMAND osmium_pub_names ${CMAKE_CURRENT_SOURCE_DIR}/pubs.osm)

set_tests_properties(examples_pub_names PROPERTIES
                     PASS_REGULAR_EXPRESSION "^Im Holze\n$")

