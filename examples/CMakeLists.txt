message(STATUS "Configuring examples...")

if(GETOPT_MISSING)
    message(FATAL_ERROR "On Visual Studio the wingetopt library is needed to compile examples")
endif()

set(SIMPLE_EXAMPLES
    osmium_convert
    osmium_count
    osmium_create_node_cache
    osmium_debug
    osmium_read
    osmium_serdump
    osmium_use_node_cache
)

if(SPARSEHASH_FOUND)
    list(APPEND SIMPLE_EXAMPLES osmium_area_test)
else()
    message("Skipped osmium_area_test example - needed Google SparseHash")
endif()

# In 'Dev' mode: compile with very strict warnings and turn them into errors.
if(CMAKE_BUILD_TYPE STREQUAL "Dev")
    add_definitions(-Werror ${OSMIUM_WARNING_OPTIONS})
endif()

foreach(example ${SIMPLE_EXAMPLES})
    message(STATUS "Configuring example ${example}...")
    add_executable(${example} "${example}.cpp")
    target_link_libraries(${example} ${OSMIUM_LIBRARIES})
endforeach(example)

unset(Boost_LIBRARIES)
unset(Boost_FOUND)
find_package(Boost 1.38 COMPONENTS program_options)

if(Boost_PROGRAM_OPTIONS_FOUND)
    set(example
        osmium_index
    )
    message(STATUS "Configuring example ${example}...")
    add_executable(${example} "${example}.cpp")
    target_link_libraries(${example} ${Boost_PROGRAM_OPTIONS_LIBRARY} ${OSMIUM_LIBRARIES})
else()
    message("Can not build osmium_index example without Boost program_options")
endif()

set(OGR_EXAMPLES
    osmium_toogr
    osmium_toogr2
    osmium_toogr2_exp
)

if(GDAL_FOUND AND GEOS_FOUND AND SPARSEHASH_FOUND)
    foreach(example ${OGR_EXAMPLES})
        message(STATUS "Configuring example ${example}...")
        add_executable(${example} "${example}.cpp")
        target_link_libraries(${example} ${OSMIUM_LIBRARIES})
    endforeach(example)
else()
    message("GDAL+GEOS+SparseHash not found, skipping OGR examples")
endif()

