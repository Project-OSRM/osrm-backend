
add_test(NAME examples_create_pois_okay
         COMMAND osmium_create_pois -)

set_tests_properties(examples_create_pois_okay PROPERTIES
                     PASS_REGULAR_EXPRESSION "^n-1 v1 dV c0 t....-..-..T..:..:..Z i0 u Tamenity=post_box x1\\.23 y3\\.45\nn-2 v1 dV c0 t....-..-..T..:..:..Z i0 u Tamenity=restaurant,name=Chez%20%OSM x1\\.24 y3\\.46\n$"
)

add_test(NAME examples_create_pois_unknown_file_type
         COMMAND osmium_create_pois foo)

set_tests_properties(examples_create_pois_unknown_file_type PROPERTIES WILL_FAIL true)

