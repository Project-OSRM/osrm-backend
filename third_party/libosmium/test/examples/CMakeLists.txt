#-----------------------------------------------------------------------------
#
#  CMake Config
#
#  Libosmium example tests
#
#-----------------------------------------------------------------------------

message(STATUS "Configuring example tests")

file(GLOB _dirs RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/t/*)

foreach(_dir ${_dirs})
    message(STATUS "  adding test: ${_dir}")
    add_subdirectory("${_dir}")
endforeach()

message(STATUS "Configuring example tests - done")


#-----------------------------------------------------------------------------
