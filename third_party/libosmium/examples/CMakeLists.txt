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
    create_node_cache
    debug
    filter_discussions
    index
    read
    serdump
    use_node_cache
    CACHE STRING "Example programs"
)


#-----------------------------------------------------------------------------
#
#  Examples depending on wingetopt
#
#-----------------------------------------------------------------------------
set(GETOPT_EXAMPLES area_test convert serdump)
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
#  Examples depending on SparseHash
#
#-----------------------------------------------------------------------------
if(NOT SPARSEHASH_FOUND)
    list(REMOVE_ITEM EXAMPLES area_test)
    message(STATUS "Configuring examples - Skipping examples because Google SparseHash not found:")
    message(STATUS "  - osmium_area_test")
endif()


#-----------------------------------------------------------------------------
#
#  Examples depending on Boost Program Options
#
#-----------------------------------------------------------------------------
unset(Boost_LIBRARIES)
unset(Boost_FOUND)
find_package(Boost 1.38 COMPONENTS program_options)

if(Boost_PROGRAM_OPTIONS_FOUND)
    list(APPEND EXAMPLE_LIBS_index ${Boost_PROGRAM_OPTIONS_LIBRARY})
else()
    list(REMOVE_ITEM EXAMPLES index)
    message(STATUS "Configuring examples - Skipping examples because Boost program_options not found:")
    message(STATUS "  - osmium_index")
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
