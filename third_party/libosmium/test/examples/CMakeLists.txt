#-----------------------------------------------------------------------------
#
#  CMake Config
#
#  Libosmium example tests
#
#-----------------------------------------------------------------------------

message(STATUS "Configuring example tests")

set(EXAMPLE_TESTS
    amenity_list
    area_test
    change_tags
    convert
    count
    create_pois
    debug
    dump_internal
    filter_discussions
    index_lookup
    location_cache
    pub_names
    read
    read_with_progress
    road_length
    tiles
)

# This is the list of all tests that are in the repository. It should be the
# same as the configured list above, if not we'll find out about that below
# and create a fatal error.
file(GLOB _dirs RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/t ${CMAKE_CURRENT_SOURCE_DIR}/t/*)

foreach(_dir ${EXAMPLE_TESTS})
    message(STATUS "  adding test: ${_dir}")
    add_subdirectory("t/${_dir}")
    list(REMOVE_ITEM _dirs ${_dir})
endforeach()

if(NOT ${_dirs} STREQUAL "")
    message(FATAL_ERROR "Found example tests that are not configured: ${_dirs}")
endif()

message(STATUS "Configuring example tests - done")


#-----------------------------------------------------------------------------
