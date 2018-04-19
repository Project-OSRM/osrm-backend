#-----------------------------------------------------------------------------
#
#  CMake Config
#
#  protozero documentation
#
#-----------------------------------------------------------------------------

message(STATUS "Configuring documentation")

message(STATUS "Looking for doxygen")
find_package(Doxygen)

if(DOXYGEN_FOUND)
    message(STATUS "Looking for doxygen - found")
    configure_file(Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)

    file(GLOB HEADER_FILES "${CMAKE_SOURCE_DIR}/include/protozero/*.hpp")
    add_custom_command(OUTPUT html/index.html
        COMMAND ${DOXYGEN_EXECUTABLE}
        ARGS ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
        DEPENDS Doxyfile.in advanced.md cheatsheet.md tutorial.md
                ${HEADER_FILES}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Generating API documentation with Doxygen" VERBATIM)
    add_custom_target(doc ALL
                      DEPENDS html/index.html)
else()
    message(STATUS "Looking for doxygen - not found")
    message(STATUS "  Disabled making of documentation.")
endif()

#-----------------------------------------------------------------------------
message(STATUS "Configuring documentation - done")


#-----------------------------------------------------------------------------
