
add_test(NAME examples_count
         COMMAND osmium_count ${CMAKE_CURRENT_SOURCE_DIR}/data.osm)

set_tests_properties(examples_count PROPERTIES
                     PASS_REGULAR_EXPRESSION "^Nodes: 2\nWays: 1\nRelations: 1\n\nMemory used:"
)

