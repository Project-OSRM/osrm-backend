#-----------------------------------------------------------------------------
#
#  CMake Config
#
#  Libosmium examples
#
#-----------------------------------------------------------------------------

message(STATUS "Configuring examples")

set(EXAMPLES
    area_test
    convert
    count
    debug
    filter_discussions
    index
    location_cache_create
    location_cache_use
    pub_names
    read
    read_with_progress
    road_length
    serdump
    tiles
    CACHE STRING "Example programs"
)


#-----------------------------------------------------------------------------
#
#  Examples depending on wingetopt
#
#-----------------------------------------------------------------------------
set(GETOPT_EXAMPLES area_test convert index serdump)
if(NOT GETOPT_MISSING)
    foreach(example ${GETOPT_EXAMPLES})
        list(APPEND EXAMPLE_LIBS_${example} ${GETOPT_LIBRARY})
    endforeach()
else()
    message(STATUS "Configuring examples - Skipping examples because on Visual Studio the wingetopt library is needed and was not found:")
    foreach(example ${GETOPT_EXAMPLES})
        message(STATUS "  - osmium_${example}")
        list(REMOVE_ITEM EXAMPLES ${example})
    endforeach()
endif()


#-----------------------------------------------------------------------------
#
#  Configure examples
#
#-----------------------------------------------------------------------------
message(STATUS "Configuring examples - Building these examples:")
foreach(example ${EXAMPLES})
    message(STATUS "  - osmium_${example}")
    add_executable(osmium_${example} "osmium_${example}.cpp")
    target_link_libraries(osmium_${example} ${OSMIUM_IO_LIBRARIES} ${EXAMPLE_LIBS_${example}})
endforeach()


#-----------------------------------------------------------------------------
message(STATUS "Configuring examples - done")


#-----------------------------------------------------------------------------
