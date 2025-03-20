#-----------------------------------------------------------------------------
#
#  CMake Config
#
#  Libosmium examples
#
#-----------------------------------------------------------------------------

message(STATUS "Configuring examples")

set(EXAMPLES
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
    location_cache_create
    location_cache_use
    pub_names
    read
    read_with_progress
    road_length
    tags_filter
    tiles
    CACHE STRING "Example programs"
)


#-----------------------------------------------------------------------------
#
#  Configure examples
#
#-----------------------------------------------------------------------------
message(STATUS "Configuring examples - Building these examples:")
foreach(example ${EXAMPLES})
    message(STATUS "  - osmium_${example}")
    add_executable(osmium_${example} "osmium_${example}.cpp")
    set_pthread_on_target(osmium_${example})
    target_link_libraries(osmium_${example} ${OSMIUM_IO_LIBRARIES} ${EXAMPLE_LIBS_${example}})
    add_test(NAME examples_usage_${example} COMMAND osmium_${example})
    set_tests_properties(examples_usage_${example} PROPERTIES
                         PASS_REGULAR_EXPRESSION "^Usage: "
    )
endforeach()


#-----------------------------------------------------------------------------
message(STATUS "Configuring examples - done")


#-----------------------------------------------------------------------------
