
add_test(NAME examples_index_lookup_help
         COMMAND osmium_index_lookup -h)

set_tests_properties(examples_index_lookup_help PROPERTIES
                     PASS_REGULAR_EXPRESSION "^Usage: osmium_index_lookup")

# Fails with message if index file doesn't exist
add_test(NAME examples_index_lookup_no_file
         COMMAND osmium_index_lookup --list=file_does_not_exist --type=location --dump)

set_tests_properties(examples_index_lookup_no_file PROPERTIES
                     PASS_REGULAR_EXPRESSION "^Can not open file")


# Fails with message if --type option is not used
add_test(NAME examples_index_lookup_no_type_option
         COMMAND osmium_index_lookup --list=file_does_not_exist --dump)

set_tests_properties(examples_index_lookup_no_type_option PROPERTIES
                     PASS_REGULAR_EXPRESSION "^Need --type argument.")


# Fails with message if --type option is used with unknown type
add_test(NAME examples_index_lookup_unknown_type
         COMMAND osmium_index_lookup --list=file_does_not_exist --type=UNKNOWN --dump)

set_tests_properties(examples_index_lookup_unknown_type PROPERTIES
                     PASS_REGULAR_EXPRESSION "^Unknown type 'UNKNOWN'")


# Fails with message when combining options --array and --list
add_test(NAME examples_index_lookup_array_list
         COMMAND osmium_index_lookup --list=a --array=b)

set_tests_properties(examples_index_lookup_array_list PROPERTIES
                     PASS_REGULAR_EXPRESSION "^Need option --array or --list, but not both\n$")


# Fails with message when combining options --dump and --search
add_test(NAME examples_index_lookup_dump_search
         COMMAND osmium_index_lookup --list=x --dump --search=123)

set_tests_properties(examples_index_lookup_dump_search PROPERTIES
                     PASS_REGULAR_EXPRESSION "Need option --dump or --search, but not both")

