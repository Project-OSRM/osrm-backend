
add_test(NAME examples_convert_help
         COMMAND osmium_convert -h)

set_tests_properties(examples_convert_help PROPERTIES
                     PASS_REGULAR_EXPRESSION "^osmium_convert .*OUTFILE")


add_test(NAME examples_convert_xml_debug
         COMMAND osmium_convert -t debug ${CMAKE_CURRENT_SOURCE_DIR}/data.osm)

set_tests_properties(examples_convert_xml_debug PROPERTIES
                     PASS_REGULAR_EXPRESSION "node 701000 visible\n  version:   1")


add_test(NAME examples_convert_xml_opl
         COMMAND osmium_convert -f osm -t opl ${CMAKE_CURRENT_SOURCE_DIR}/data.osm)

set_tests_properties(examples_convert_xml_opl PROPERTIES
                     PASS_REGULAR_EXPRESSION "n701001 v1 dV c1 t2014-01-01T00:00:00Z i1 utest T x7.11 y1.04")


add_test(NAME examples_convert_xml_opl_long1
         COMMAND osmium_convert --from-format osm --to-format opl ${CMAKE_CURRENT_SOURCE_DIR}/data.osm)

set_tests_properties(examples_convert_xml_opl_long1 PROPERTIES
                     PASS_REGULAR_EXPRESSION "n701001 v1 dV c1 t2014-01-01T00:00:00Z i1 utest T x7.11 y1.04")


add_test(NAME examples_convert_xml_opl_long2
         COMMAND osmium_convert --from-format=osm --to-format=opl ${CMAKE_CURRENT_SOURCE_DIR}/data.osm)

set_tests_properties(examples_convert_xml_opl_long2 PROPERTIES
                     PASS_REGULAR_EXPRESSION "n701001 v1 dV c1 t2014-01-01T00:00:00Z i1 utest T x7.11 y1.04")


add_test(NAME examples_convert_xml_pbf
         COMMAND osmium_convert -t pbf ${CMAKE_CURRENT_SOURCE_DIR}/data.osm -)


add_test(NAME examples_convert_xml_xml
         COMMAND osmium_convert -t xml ${CMAKE_CURRENT_SOURCE_DIR}/data.osm)

set_tests_properties(examples_convert_xml_xml PROPERTIES
                     PASS_REGULAR_EXPRESSION "<node id=\"701001\" ")

# Should give a warning when converting from history to non-history file
add_test(NAME examples_convert_osh_xml
         COMMAND osmium_convert -f osh -t osm ${CMAKE_CURRENT_SOURCE_DIR}/data.osm)

set_tests_properties(examples_convert_osh_xml PROPERTIES
                     PASS_REGULAR_EXPRESSION "Warning! You are converting from an OSM file")

# Should fail when an unknown command line option is used
add_test(NAME examples_convert_unknown_option
         COMMAND osmium_convert --unknown)

set_tests_properties(examples_convert_unknown_option PROPERTIES
                     WILL_FAIL true)

