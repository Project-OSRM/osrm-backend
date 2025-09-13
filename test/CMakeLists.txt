#-----------------------------------------------------------------------------
#
#  CMake config
#
#  protozero tests
#
#-----------------------------------------------------------------------------

include_directories(SYSTEM "${CMAKE_CURRENT_SOURCE_DIR}/catch")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")

add_subdirectory(unit)

set(TEST_DIRS alignment
              bool
              bytes
              complex
              double
              enum
              fixed32
              fixed64
              float
              int32
              int64
              message
              nested
              repeated
              repeated_packed_bool
              repeated_packed_double
              repeated_packed_enum
              repeated_packed_fixed32
              repeated_packed_fixed64
              repeated_packed_float
              repeated_packed_int32
              repeated_packed_int64
              repeated_packed_sfixed32
              repeated_packed_sfixed64
              repeated_packed_sint32
              repeated_packed_sint64
              repeated_packed_uint32
              repeated_packed_uint64
              rollback
              sfixed32
              sfixed64
              sint32
              sint64
              skip
              string
              tag_and_type
              tags
              uint32
              uint64
              vector_tile
              wrong_type_access)

string(REGEX REPLACE "([^;]+)" "t/\\1/reader_test_cases.cpp" _test_sources "${TEST_DIRS}")

add_executable(reader_tests reader_tests.cpp ${_test_sources})

add_test(NAME reader_tests COMMAND reader_tests)

set_tests_properties(reader_tests PROPERTIES WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

if(PROTOBUF_FOUND)
    message(STATUS "Found protobuf libraries: Adding writer tests...")

    include_directories(SYSTEM ${PROTOBUF_INCLUDE_DIRS} ${CMAKE_CURRENT_BINARY_DIR})

    set(PROTOBUF_GENERATE_CPP_APPEND_PATH false)

    foreach(_dir IN LISTS TEST_DIRS)
        set(_full_src_dir "${CMAKE_CURRENT_SOURCE_DIR}/t/${_dir}")
        if(EXISTS "${_full_src_dir}/writer_test_cases.cpp")
            message(STATUS "  Adding ${_dir}")
            set(_full_bin_dir "${CMAKE_CURRENT_BINARY_DIR}/t/${_dir}")
            set(_proto_file "${_full_src_dir}/${_dir}_testcase.proto")
            set(_src_file "${_full_bin_dir}/${_dir}_testcase.pb.cc")
            set(_hdr_file "${_full_bin_dir}/${_dir}_testcase.pb.h")

            file(MAKE_DIRECTORY ${_full_bin_dir})

            list(APPEND SOURCES     "${_full_src_dir}/writer_test_cases.cpp")
            list(APPEND PROTO_FILES "${_proto_file}")
            list(APPEND PROTO_SRCS  "${_src_file}")
            list(APPEND PROTO_HDRS  "${_hdr_file}")

            set_source_files_properties(${_proto_file} ${_hdr_file}
                                        PROPERTIES GENERATED TRUE)

            add_custom_command(
                OUTPUT ${_src_file} ${_hdr_file}
                COMMAND ${PROTOBUF_PROTOC_EXECUTABLE}
                ARGS --cpp_out=${_full_bin_dir} -I ${_full_src_dir} ${_proto_file}
                DEPENDS ${_proto_file}
                VERBATIM)
        endif()
    endforeach()

    add_executable(writer_tests writer_tests.cpp ${SOURCES} ${PROTO_SRCS} ${PROTO_HDRS})

    target_compile_features(writer_tests PUBLIC cxx_std_14)
    target_link_libraries(writer_tests PRIVATE protobuf::libprotobuf-lite)

    if(NOT MSVC)
        set_target_properties(writer_tests PROPERTIES COMPILE_FLAGS "-pthread")
        if(NOT APPLE)
            set_target_properties(writer_tests PROPERTIES LINK_FLAGS "-pthread")
        endif()
    endif()

    add_test(NAME writer_tests COMMAND writer_tests)
else()
    message(STATUS "Protobuf libraries not found: Disabling writer tests.")
endif()


#-----------------------------------------------------------------------------
