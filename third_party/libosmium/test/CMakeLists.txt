#-----------------------------------------------------------------------------
#
#  CMake Config
#
#  Libosmium unit tests
#
#-----------------------------------------------------------------------------

message(STATUS "Configuring unit tests")

include(CMakeParseArguments)

include_directories(SYSTEM catch)
include_directories(include)

add_library(testlib STATIC test_main.cpp)

set(ALL_TESTS "")

# Otherwise GCC throws a lot of warnings for REQUIRE(...) from Catch v.1.2.1
if(CMAKE_COMPILER_IS_GNUCXX)
    add_definitions(-Wno-parentheses)
endif()

if(NOT MSVC)
    add_definitions(-Wno-deprecated-declarations)
endif()


#-----------------------------------------------------------------------------
#
#  Define function for adding tests
#
#  add_unit_tests(group name [ENABLE_IF bool] [LIBS libs] [LABELS labels])
#
#  group  - test group (directory)
#  name   - name of test
#  bool   - boolean variable telling whether the test should be run (optional)
#  libs   - lib or libs that should be used when compiling test (optional)
#  labels - additional labels this test should get (optional)
#
#-----------------------------------------------------------------------------
function(add_unit_test _tgroup _tname)
    set(_testid "${_tgroup}_${_tname}")
    set(_tpath "${_tgroup}/${_tname}")

    set(ALL_TESTS "${ALL_TESTS};${_tpath}" PARENT_SCOPE)

    cmake_parse_arguments(_param "" "ENABLE_IF" "LIBS;LABELS" ${ARGN})

    if(Osmium_DEBUG)
        message("${_testid} ENABLE_IF=[${_param_ENABLE_IF}] LIBS=[${_param_LIBS}] LABELS=[${_param_LABELS}]")
    endif()

    if((NOT(DEFINED _param_ENABLE_IF)) OR (_param_ENABLE_IF))
        if(Osmium_DEBUG)
            message("Adding test: ${_tpath}")
        endif()
        add_executable(${_testid} t/${_tpath}.cpp)
        target_link_libraries(${_testid} testlib)

        if(DEFINED _param_LIBS)
            if(Osmium_DEBUG)
                message("  Adding libs: ${_param_LIBS}")
            endif()
            target_link_libraries(${_testid} ${_param_LIBS})
            if(_param_LIBS MATCHES "lpthread")
                set_pthread_on_target(${_testid})
            endif()
        endif()

        add_test(NAME ${_testid}
                 WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                 COMMAND ${_testid}
        )

        set(_labels "unit;fast;${_tgroup}")
        if(DEFINED _param_LABELS)
            if(Osmium_DEBUG)
                message("  Adding labels: ${_param_LABELS}")
            endif()
            set(_labels "${_labels};${_param_LABELS}")
        endif()

        set_tests_properties(${_testid} PROPERTIES
            LABELS "${_labels}"
            ENVIRONMENT "OSMIUM_TEST_DATA_DIR=${CMAKE_CURRENT_SOURCE_DIR}"
        )
    else()
        message("Skipped test ${_tpath} because a dependency was disabled/not found")
        set(OSMIUM_SKIPPED_TESTS
            "${OSMIUM_SKIPPED_TESTS} ${_tpath}"
            CACHE STRING "Tests that were skipped because of missing dependecies")
    endif()
endfunction()


#-----------------------------------------------------------------------------
#
#  Prepare some variables so querying it for tests work properly.
#
#-----------------------------------------------------------------------------

if(NOT GEOS_FOUND)
    set(GEOS_FOUND FALSE)
endif()

if(NOT PROJ_FOUND)
    set(PROJ_FOUND FALSE)
endif()

if(NOT SPARSEHASH_FOUND)
    set(SPARSEHASH_FOUND FALSE)
endif()

if(NOT BZIP2_FOUND)
    set(BZIP2_FOUND FALSE)
endif()

if(NOT Threads_FOUND)
    set(Threads_FOUND FALSE)
endif()


#-----------------------------------------------------------------------------
#
#  Add all tests.
#
#-----------------------------------------------------------------------------
add_unit_test(area test_area_id)
add_unit_test(area test_assembler)
add_unit_test(area test_node_ref_segment)

add_unit_test(osm test_area ENABLE_IF ${ZLIB_FOUND} LIBS ${ZLIB_LIBRARIES})
add_unit_test(osm test_box ENABLE_IF ${ZLIB_FOUND} LIBS ${ZLIB_LIBRARIES})
add_unit_test(osm test_changeset ENABLE_IF ${ZLIB_FOUND} LIBS ${ZLIB_LIBRARIES})
add_unit_test(osm test_crc ENABLE_IF ${ZLIB_FOUND} LIBS ${ZLIB_LIBRARIES})
add_unit_test(osm test_entity_bits)
add_unit_test(osm test_location)
add_unit_test(osm test_metadata)
add_unit_test(osm test_node ENABLE_IF ${ZLIB_FOUND} LIBS ${ZLIB_LIBRARIES})
add_unit_test(osm test_node_ref)
add_unit_test(osm test_object_comparisons)
add_unit_test(osm test_relation ENABLE_IF ${ZLIB_FOUND} LIBS ${ZLIB_LIBRARIES})
add_unit_test(osm test_timestamp)
add_unit_test(osm test_types_from_string)
add_unit_test(osm test_way ENABLE_IF ${ZLIB_FOUND} LIBS ${ZLIB_LIBRARIES})

add_unit_test(memory test_buffer_basics)
add_unit_test(memory test_buffer_node)
add_unit_test(memory test_buffer_purge)
add_unit_test(memory test_callback_buffer)
add_unit_test(memory test_item)
add_unit_test(memory test_type_is_compatible)

add_unit_test(builder test_attr)
add_unit_test(builder test_object_builder)

add_unit_test(geom test_coordinates)
add_unit_test(geom test_crs ENABLE_IF ${PROJ_FOUND} LIBS ${PROJ_LIBRARY})
add_unit_test(geom test_exception)
add_unit_test(geom test_factory_with_projection ENABLE_IF ${PROJ_FOUND} LIBS ${PROJ_LIBRARY})
add_unit_test(geom test_geojson)
add_unit_test(geom test_geos ENABLE_IF ${GEOS_FOUND} LIBS ${GEOS_LIBRARY})
add_unit_test(geom test_mercator)
add_unit_test(geom test_ogr ENABLE_IF ${GDAL_FOUND} LIBS ${GDAL_LIBRARY})
add_unit_test(geom test_ogr_wkb ENABLE_IF ${GDAL_FOUND} LIBS ${GDAL_LIBRARY})
add_unit_test(geom test_projection ENABLE_IF ${PROJ_FOUND} LIBS ${PROJ_LIBRARY})
add_unit_test(geom test_tile)
add_unit_test(geom test_wkb)
add_unit_test(geom test_wkt)

add_unit_test(handler test_apply LIBS "${OSMIUM_XML_LIBRARIES}")
add_unit_test(handler test_check_order_handler)
add_unit_test(handler test_dynamic_handler)

add_unit_test(index test_dump_and_load_index)
add_unit_test(index test_dump_sparse_as_array)
add_unit_test(index test_file_based_index)
add_unit_test(index test_id_set)
add_unit_test(index test_id_to_location ENABLE_IF ${SPARSEHASH_FOUND})
add_unit_test(index test_nwr_array)
add_unit_test(index test_object_pointer_collection)
add_unit_test(index test_relations_map)

add_unit_test(io test_compression_factory)
add_unit_test(io test_file_formats)
add_unit_test(io test_nocompression)
add_unit_test(io test_output_utils)
add_unit_test(io test_string_table)

add_unit_test(io test_bzip2 ENABLE_IF ${BZIP2_FOUND} LIBS ${BZIP2_LIBRARIES})
add_unit_test(io test_gzip ENABLE_IF ${ZLIB_FOUND} LIBS ${ZLIB_LIBRARIES})
add_unit_test(io test_opl_parser ENABLE_IF ${Threads_FOUND} LIBS ${CMAKE_THREAD_LIBS_INIT})
add_unit_test(io test_output_iterator ENABLE_IF ${Threads_FOUND} LIBS ${CMAKE_THREAD_LIBS_INIT})
add_unit_test(io test_pbf ENABLE_IF ${Threads_FOUND} LIBS ${OSMIUM_PBF_LIBRARIES})
add_unit_test(io test_reader LIBS "${OSMIUM_XML_LIBRARIES};${OSMIUM_PBF_LIBRARIES}")
add_unit_test(io test_reader_fileformat ENABLE_IF ${Threads_FOUND} LIBS ${CMAKE_THREAD_LIBS_INIT})
add_unit_test(io test_reader_with_mock_decompression ENABLE_IF ${Threads_FOUND} LIBS ${OSMIUM_XML_LIBRARIES})
add_unit_test(io test_reader_with_mock_parser ENABLE_IF ${Threads_FOUND} LIBS ${CMAKE_THREAD_LIBS_INIT})
add_unit_test(io test_writer ENABLE_IF ${Threads_FOUND} LIBS ${OSMIUM_XML_LIBRARIES})
add_unit_test(io test_writer_with_mock_compression ENABLE_IF ${Threads_FOUND} LIBS ${OSMIUM_XML_LIBRARIES})
add_unit_test(io test_writer_with_mock_encoder ENABLE_IF ${Threads_FOUND} LIBS ${OSMIUM_XML_LIBRARIES})

add_unit_test(relations test_members_database)
add_unit_test(relations test_read_relations ENABLE_IF ${Threads_FOUND} LIBS ${OSMIUM_XML_LIBRARIES})
add_unit_test(relations test_relations_database)
add_unit_test(relations test_relations_manager ENABLE_IF ${Threads_FOUND} LIBS ${OSMIUM_XML_LIBRARIES})

add_unit_test(storage test_item_stash)

add_unit_test(tags test_filter)
add_unit_test(tags test_operators)
add_unit_test(tags test_tag_list)
add_unit_test(tags test_tag_matcher)
add_unit_test(tags test_tags_filter)

add_unit_test(thread test_pool ENABLE_IF ${Threads_FOUND} LIBS ${CMAKE_THREAD_LIBS_INIT})
add_unit_test(thread test_queue ENABLE_IF ${Threads_FOUND} LIBS ${CMAKE_THREAD_LIBS_INIT})
add_unit_test(thread test_util ENABLE_IF ${Threads_FOUND} LIBS ${CMAKE_THREAD_LIBS_INIT})

add_unit_test(util test_config)
add_unit_test(util test_delta)
add_unit_test(util test_double)
add_unit_test(util test_file)
add_unit_test(util test_memory)
add_unit_test(util test_memory_mapping)
add_unit_test(util test_minmax)
add_unit_test(util test_misc)
add_unit_test(util test_options)
add_unit_test(util test_string)
add_unit_test(util test_string_matcher)
add_unit_test(util test_timer_disabled)
add_unit_test(util test_timer_enabled)


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
