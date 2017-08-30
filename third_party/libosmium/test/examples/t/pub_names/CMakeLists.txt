
add_test(NAME examples_pub_names_node
         COMMAND osmium_pub_names ${CMAKE_CURRENT_SOURCE_DIR}/pub-node.osm)
set_tests_properties(examples_pub_names_node PROPERTIES
                     PASS_REGULAR_EXPRESSION "^Im Holze\n$")

add_test(NAME examples_pub_names_way
         COMMAND osmium_pub_names ${CMAKE_CURRENT_SOURCE_DIR}/pub-way.osm)
set_tests_properties(examples_pub_names_way PROPERTIES
                     PASS_REGULAR_EXPRESSION "^Vereinsheim\n$")

add_test(NAME examples_pub_names_noname
         COMMAND osmium_pub_names ${CMAKE_CURRENT_SOURCE_DIR}/pub-noname.osm)
set_tests_properties(examples_pub_names_noname PROPERTIES
                     PASS_REGULAR_EXPRESSION "^pub with unknown name\n$")

add_test(NAME examples_pub_names_addr
         COMMAND osmium_pub_names ${CMAKE_CURRENT_SOURCE_DIR}/pub-addr.osm)
set_tests_properties(examples_pub_names_addr PROPERTIES
                     PASS_REGULAR_EXPRESSION "^Im Holze\n  addr:city: Bremen\n")

