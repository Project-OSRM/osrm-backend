
add_test(NAME examples_change_tags
         COMMAND osmium_change_tags ${CMAKE_CURRENT_SOURCE_DIR}/data.osm ${CMAKE_CURRENT_BINARY_DIR}/result.osm)

add_test(NAME examples_change_tags_compare
         COMMAND ${CMAKE_COMMAND} -E compare_files ${CMAKE_CURRENT_SOURCE_DIR}/result.osm ${CMAKE_CURRENT_BINARY_DIR}/result.osm)

set_tests_properties(examples_change_tags_compare PROPERTIES DEPENDS example_change_tags)

