
add_test(NAME examples_location_cache_create
         COMMAND osmium_location_cache_create ${CMAKE_CURRENT_SOURCE_DIR}/data.osm ${CMAKE_CURRENT_BINARY_DIR}/locations.idx)


# Fails with message if index file doesn't exist
add_test(NAME examples_location_cache_no_file
         COMMAND osmium_location_cache_use ${CMAKE_CURRENT_SOURCE_DIR}/way.osm ${CMAKE_CURRENT_BINARY_DIR}/file_does_not_exist)

set_tests_properties(examples_location_cache_no_file PROPERTIES
                     PASS_REGULAR_EXPRESSION "Can not open location cache file")


add_test(NAME examples_location_cache_use
         COMMAND osmium_location_cache_use ${CMAKE_CURRENT_SOURCE_DIR}/way.osm ${CMAKE_CURRENT_BINARY_DIR}/locations.idx)

set_tests_properties(examples_location_cache_use PROPERTIES
                     DEPENDS examples_location_cache_create
                     PASS_REGULAR_EXPRESSION "^way 20\n  node 10 \\(7.11,1.01\\)\n  node 11 \\(7.11,1.04\\)\n  node 12 \\(7.14,1.04\\)\n$")


add_test(NAME examples_location_cache_dump
         COMMAND osmium_index_lookup --list=${CMAKE_CURRENT_BINARY_DIR}/locations.idx --type=location --dump)

set_tests_properties(examples_location_cache_dump PROPERTIES
                     DEPENDS examples_location_cache_create
                     PASS_REGULAR_EXPRESSION "^10 \\(7.11,1.01\\)\n11 \\(7.11,1.04\\)\n12 \\(7.14,1.04\\)\n13 \\(7.14,1.01\\)\n$")


add_test(NAME examples_location_cache_search
         COMMAND osmium_index_lookup --list=${CMAKE_CURRENT_BINARY_DIR}/locations.idx --type=location --search=12)

set_tests_properties(examples_location_cache_search PROPERTIES
                     DEPENDS examples_location_cache_create
                     PASS_REGULAR_EXPRESSION "^12 \\(7.14,1.04\\)\n$")

