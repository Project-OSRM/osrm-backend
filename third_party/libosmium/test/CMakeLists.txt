#-----------------------------------------------------------------------------
#
#  CMake Config
#
#  Libosmium unit tests
#
#-----------------------------------------------------------------------------

message(STATUS "Configuring unit tests")

include_directories(include)

add_library(testlib STATIC test_main.cpp)

set(ALL_TESTS "")


#-----------------------------------------------------------------------------
#
#  Define function for adding tests
#
#  Call with parameters:
#    TGROUP - test group (directory)
#    TNAME  - name of test
#    ARGV2  - flag to enable test (optional)
#    ARGV3  - libraries to add (optional)
#
#-----------------------------------------------------------------------------
function(add_unit_test TGROUP TNAME)
    set(ALL_TESTS "${ALL_TESTS};${TGROUP}/${TNAME}" PARENT_SCOPE)
    if((${ARGC} EQUAL 2) OR (${ARGV2}))
        if(Osmium_DEBUG)
            message("Adding test ${TGROUP}/${TNAME}")
        endif()
        set(TESTNAME "${TGROUP}_${TNAME}")
        add_executable(${TESTNAME} t/${TGROUP}/${TNAME}.cpp)
        target_link_libraries(${TESTNAME} testlib)
        if((${ARGV2}) AND (DEFINED ARGV3))
            if(Osmium_DEBUG)
                message("  Adding libs ${ARGV3}")
            endif()
            target_link_libraries(${TESTNAME} ${ARGV3})
        endif()
        add_test(NAME ${TESTNAME}
                 WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                 COMMAND ${TESTNAME}
        )
        set_tests_properties(${TESTNAME} PROPERTIES
            LABELS "unit;fast;${TGROUP}"
            ENVIRONMENT "OSMIUM_TEST_DATA_DIR=${CMAKE_CURRENT_SOURCE_DIR}"
        )
    else()
        message("Skipped test ${TGROUP}/${TNAME} because a dependency was not found")
        set(OSMIUM_SKIPPED_TESTS
            "${OSMIUM_SKIPPED_TESTS} ${TGROUP}/${TNAME}"
            CACHE STRING "Tests that were skipped because of missing dependecies")
    endif()
endfunction()

#-----------------------------------------------------------------------------
#
#  Add all tests.
#
#-----------------------------------------------------------------------------
add_unit_test(area test_area_id)
add_unit_test(area test_node_ref_segment)

add_unit_test(basic test_box)
add_unit_test(basic test_changeset)
add_unit_test(basic test_entity_bits)
add_unit_test(basic test_location)
add_unit_test(basic test_node)
add_unit_test(basic test_node_ref)
add_unit_test(basic test_object_comparisons)
add_unit_test(basic test_relation)
add_unit_test(basic test_timestamp)
add_unit_test(basic test_way)

add_unit_test(buffer test_buffer_node)
add_unit_test(buffer test_buffer_purge)

if(GEOS_FOUND AND PROJ_FOUND)
    set(GEOS_AND_PROJ_FOUND TRUE)
else()
    set(GEOS_AND_PROJ_FOUND FALSE)
endif()
add_unit_test(geom test_factory_with_projection
    ${GEOS_AND_PROJ_FOUND}
    "${GEOS_LIBRARY};${PROJ_LIBRARY}")

add_unit_test(geom test_geojson)
add_unit_test(geom test_geos ${GEOS_FOUND} ${GEOS_LIBRARY})
add_unit_test(geom test_geos_wkb ${GEOS_FOUND} ${GEOS_LIBRARY})
add_unit_test(geom test_mercator)
add_unit_test(geom test_ogr ${GDAL_FOUND} ${GDAL_LIBRARY})
add_unit_test(geom test_projection ${PROJ_FOUND} ${PROJ_LIBRARY})
add_unit_test(geom test_wkb)
add_unit_test(geom test_wkt)

add_unit_test(index test_id_to_location ${SPARSEHASH_FOUND})
add_unit_test(index test_typed_mmap)

add_unit_test(io test_bzip2 ${BZIP2_FOUND} ${BZIP2_LIBRARIES})
add_unit_test(io test_file_formats)
add_unit_test(io test_reader TRUE "${OSMIUM_XML_LIBRARIES}")
add_unit_test(io test_output_iterator ${Threads_FOUND} ${CMAKE_THREAD_LIBS_INIT})

add_unit_test(tags test_filter)
add_unit_test(tags test_operators)
add_unit_test(tags test_tag_list)

add_unit_test(thread test_pool ${Threads_FOUND} ${CMAKE_THREAD_LIBS_INIT})

add_unit_test(util test_cast_with_assert)
add_unit_test(util test_double)
add_unit_test(util test_options)
add_unit_test(util test_string)


#-----------------------------------------------------------------------------
#
#  Check that all tests available in test/t/*/test_*.cpp are run.
#
#-----------------------------------------------------------------------------
file(GLOB TESTS_IN_DIR RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}/t" t/*/test_*.cpp)

foreach(file ${TESTS_IN_DIR})
    string(REPLACE ".cpp" "" out1 ${file})
    string(REPLACE "//" "/" tname ${out1})
    list(FIND ALL_TESTS ${tname} found)
    if(${found} EQUAL -1)
        message(WARNING "Test '${tname}' not found in cmake config. It will not be run!")
    endif()
endforeach()


#-----------------------------------------------------------------------------
message(STATUS "Configuring unit tests - done")


#-----------------------------------------------------------------------------
