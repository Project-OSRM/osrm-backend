#-----------------------------------------------------------------------------
#
#  CMake config
#
#  protozero unit tests
#
#-----------------------------------------------------------------------------

set(UNIT_TESTS data_view
               basic
               endian
               exceptions
               varint
               zigzag)

string(REGEX REPLACE "([^;]+)" "test_\\1.cpp" _test_sources "${UNIT_TESTS}")

add_executable(unit_tests main.cpp ${_test_sources})

add_test(NAME unit_tests COMMAND unit_tests)


#-----------------------------------------------------------------------------
