
add_test(NAME examples_filter_discussions
         COMMAND osmium_filter_discussions ${CMAKE_CURRENT_SOURCE_DIR}/changesets.osm -)

set_tests_properties(examples_filter_discussions PROPERTIES
                     FAIL_REGULAR_EXPRESSION "<changeset id=\"402757\"")

set_tests_properties(examples_filter_discussions PROPERTIES
                     PASS_REGULAR_EXPRESSION "<changeset id=\"402758\"")

