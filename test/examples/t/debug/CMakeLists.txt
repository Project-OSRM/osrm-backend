
add_test(NAME examples_debug_all
         COMMAND osmium_debug ${CMAKE_CURRENT_SOURCE_DIR}/data.osm)

set_tests_properties(examples_debug_all PROPERTIES
                     PASS_REGULAR_EXPRESSION "  id=1\n.*  id=2\n.*  id=10\n.*  id=20\n"
)


add_test(NAME examples_debug_nodes
         COMMAND osmium_debug ${CMAKE_CURRENT_SOURCE_DIR}/data.osm n)

set_tests_properties(examples_debug_nodes PROPERTIES
                     PASS_REGULAR_EXPRESSION "id=1\n.*  id=2\n"
)

set_tests_properties(examples_debug_nodes PROPERTIES
                     FAIL_REGULAR_EXPRESSION "id=10\n"
)


add_test(NAME examples_debug_ways
         COMMAND osmium_debug ${CMAKE_CURRENT_SOURCE_DIR}/data.osm w)

set_tests_properties(examples_debug_ways PROPERTIES
                     PASS_REGULAR_EXPRESSION "  id=10\n"
)

set_tests_properties(examples_debug_ways PROPERTIES
                     FAIL_REGULAR_EXPRESSION "id=20\n"
)


add_test(NAME examples_debug_relations
         COMMAND osmium_debug ${CMAKE_CURRENT_SOURCE_DIR}/data.osm r)

set_tests_properties(examples_debug_relations PROPERTIES
                     PASS_REGULAR_EXPRESSION "  id=20\n"
)

set_tests_properties(examples_debug_relations PROPERTIES
                     FAIL_REGULAR_EXPRESSION "id=10\n"
)

add_test(NAME examples_debug_changesets
         COMMAND osmium_debug ${CMAKE_CURRENT_SOURCE_DIR}/changesets.osm c)

set_tests_properties(examples_debug_changesets PROPERTIES
                     PASS_REGULAR_EXPRESSION "  id=15449962\n"
)

set_tests_properties(examples_debug_changesets PROPERTIES
                     FAIL_REGULAR_EXPRESSION "id=10\n"
)

