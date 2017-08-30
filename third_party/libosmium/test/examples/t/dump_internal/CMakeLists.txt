
add_test(NAME examples_dump_internal
         COMMAND osmium_dump_internal ${CMAKE_CURRENT_SOURCE_DIR}/data.osm ${CMAKE_CURRENT_BINARY_DIR}/out)


add_test(NAME examples_dump_internal_index_nodes
         COMMAND osmium_index_lookup --list=${CMAKE_CURRENT_BINARY_DIR}/out/nodes.idx --type=offset --dump)

set_tests_properties(examples_dump_internal_index_nodes PROPERTIES
                     DEPENDS examples_dump_internal
                     PASS_REGULAR_EXPRESSION "^701000 .*\n701001 .*\n")


add_test(NAME examples_dump_internal_index_ways
         COMMAND osmium_index_lookup --list=${CMAKE_CURRENT_BINARY_DIR}/out/ways.idx --type=offset --dump)

set_tests_properties(examples_dump_internal_index_ways PROPERTIES
                     DEPENDS examples_dump_internal
                     PASS_REGULAR_EXPRESSION "^701800 .*\n701801 .*\n")


add_test(NAME examples_dump_internal_map_node2way_dump
         COMMAND osmium_index_lookup --list=${CMAKE_CURRENT_BINARY_DIR}/out/node2way.map --type=id --dump)

set_tests_properties(examples_dump_internal_map_node2way_dump PROPERTIES
                     DEPENDS examples_dump_internal
                     PASS_REGULAR_EXPRESSION "^701000 701800\n701000 701801\n701001 701800\n")


add_test(NAME examples_dump_internal_map_node2way_search
         COMMAND osmium_index_lookup --list=${CMAKE_CURRENT_BINARY_DIR}/out/node2way.map --type=id --search=701002)

set_tests_properties(examples_dump_internal_map_node2way_search PROPERTIES
                     DEPENDS examples_dump_internal
                     PASS_REGULAR_EXPRESSION "^701002 701800\n701002 701801\n$")

