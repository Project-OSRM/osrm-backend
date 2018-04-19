#----------------------------------------------------------------------
#
#  FindOsmium.cmake
#
#  Find the Libosmium headers and, optionally, several components needed
#  for different Libosmium functions.
#
#----------------------------------------------------------------------
#
#  Usage:
#
#    Copy this file somewhere into your project directory, where cmake can
#    find it. Usually this will be a directory called "cmake" which you can
#    add to the CMake module search path with the following line in your
#    CMakeLists.txt:
#
#      list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
#
#    Then add the following in your CMakeLists.txt:
#
#      find_package(Osmium [version] REQUIRED COMPONENTS <XXX>)
#      include_directories(SYSTEM ${OSMIUM_INCLUDE_DIRS})
#
#    The version number is optional. If it is not set, any version of
#    libosmium will do.
#
#    For the <XXX> substitute a space separated list of one or more of the
#    following components:
#
#      pbf        - include libraries needed for PBF input and output
#      xml        - include libraries needed for XML input and output
#      io         - include libraries needed for any type of input/output
#      geos       - include if you want to use any of the GEOS functions
#      gdal       - include if you want to use any of the OGR functions
#      proj       - include if you want to use any of the Proj.4 functions
#      sparsehash - include if you use the sparsehash index
#
#    You can check for success with something like this:
#
#      if(NOT OSMIUM_FOUND)
#          message(WARNING "Libosmium not found!\n")
#      endif()
#
#----------------------------------------------------------------------
#
#  Variables:
#
#    OSMIUM_FOUND         - True if Osmium found.
#    OSMIUM_INCLUDE_DIRS  - Where to find include files.
#    OSMIUM_XML_LIBRARIES - Libraries needed for XML I/O.
#    OSMIUM_PBF_LIBRARIES - Libraries needed for PBF I/O.
#    OSMIUM_IO_LIBRARIES  - Libraries needed for XML or PBF I/O.
#    OSMIUM_LIBRARIES     - All libraries Osmium uses somewhere.
#
#----------------------------------------------------------------------

# This is the list of directories where we look for osmium includes.
set(_osmium_include_path
        ../libosmium
        ~/Library/Frameworks
        /Library/Frameworks
        /opt/local # DarwinPorts
        /opt
)

# Look for the header file.
find_path(OSMIUM_INCLUDE_DIR osmium/version.hpp
    PATH_SUFFIXES include
    PATHS ${_osmium_include_path}
)

# Check libosmium version number
if(Osmium_FIND_VERSION)
    file(STRINGS "${OSMIUM_INCLUDE_DIR}/osmium/version.hpp" _libosmium_version_define REGEX "#define LIBOSMIUM_VERSION_STRING")
    if("${_libosmium_version_define}" MATCHES "#define LIBOSMIUM_VERSION_STRING \"([0-9.]+)\"")
        set(_libosmium_version "${CMAKE_MATCH_1}")
    else()
        set(_libosmium_version "unknown")
    endif()
endif()

set(OSMIUM_INCLUDE_DIRS "${OSMIUM_INCLUDE_DIR}")

#----------------------------------------------------------------------
#
#  Check for optional components
#
#----------------------------------------------------------------------
if(Osmium_FIND_COMPONENTS)
    foreach(_component ${Osmium_FIND_COMPONENTS})
        string(TOUPPER ${_component} _component_uppercase)
        set(Osmium_USE_${_component_uppercase} TRUE)
    endforeach()
endif()

#----------------------------------------------------------------------
# Component 'io' is an alias for 'pbf' and 'xml'
if(Osmium_USE_IO)
    set(Osmium_USE_PBF TRUE)
    set(Osmium_USE_XML TRUE)
endif()

#----------------------------------------------------------------------
# Component 'ogr' is an alias for 'gdal'
if(Osmium_USE_OGR)
    set(Osmium_USE_GDAL TRUE)
endif()

#----------------------------------------------------------------------
# Component 'pbf'
if(Osmium_USE_PBF)
    find_package(ZLIB)
    find_package(Threads)
    find_package(Protozero 1.5.1)

    list(APPEND OSMIUM_EXTRA_FIND_VARS ZLIB_FOUND Threads_FOUND PROTOZERO_INCLUDE_DIR)
    if(ZLIB_FOUND AND Threads_FOUND AND PROTOZERO_FOUND)
        list(APPEND OSMIUM_PBF_LIBRARIES
            ${ZLIB_LIBRARIES}
            ${CMAKE_THREAD_LIBS_INIT}
        )
        list(APPEND OSMIUM_INCLUDE_DIRS
            ${ZLIB_INCLUDE_DIR}
            ${PROTOZERO_INCLUDE_DIR}
        )
    else()
        message(WARNING "Osmium: Can not find some libraries for PBF input/output, please install them or configure the paths.")
    endif()
endif()

#----------------------------------------------------------------------
# Component 'xml'
if(Osmium_USE_XML)
    find_package(EXPAT)
    find_package(BZip2)
    find_package(ZLIB)
    find_package(Threads)

    list(APPEND OSMIUM_EXTRA_FIND_VARS EXPAT_FOUND BZIP2_FOUND ZLIB_FOUND Threads_FOUND)
    if(EXPAT_FOUND AND BZIP2_FOUND AND ZLIB_FOUND AND Threads_FOUND)
        list(APPEND OSMIUM_XML_LIBRARIES
            ${EXPAT_LIBRARIES}
            ${BZIP2_LIBRARIES}
            ${ZLIB_LIBRARIES}
            ${CMAKE_THREAD_LIBS_INIT}
        )
        list(APPEND OSMIUM_INCLUDE_DIRS
            ${EXPAT_INCLUDE_DIR}
            ${BZIP2_INCLUDE_DIR}
            ${ZLIB_INCLUDE_DIR}
        )
    else()
        message(WARNING "Osmium: Can not find some libraries for XML input/output, please install them or configure the paths.")
    endif()
endif()

#----------------------------------------------------------------------
list(APPEND OSMIUM_IO_LIBRARIES
    ${OSMIUM_PBF_LIBRARIES}
    ${OSMIUM_XML_LIBRARIES}
)

list(APPEND OSMIUM_LIBRARIES
    ${OSMIUM_IO_LIBRARIES}
)

#----------------------------------------------------------------------
# Component 'geos'
if(Osmium_USE_GEOS)
    find_path(GEOS_INCLUDE_DIR geos/geom.h)
    find_library(GEOS_LIBRARY NAMES geos)

    list(APPEND OSMIUM_EXTRA_FIND_VARS GEOS_INCLUDE_DIR GEOS_LIBRARY)
    if(GEOS_INCLUDE_DIR AND GEOS_LIBRARY)
        SET(GEOS_FOUND 1)
        list(APPEND OSMIUM_LIBRARIES ${GEOS_LIBRARY})
        list(APPEND OSMIUM_INCLUDE_DIRS ${GEOS_INCLUDE_DIR})
    else()
        message(WARNING "Osmium: GEOS library is required but not found, please install it or configure the paths.")
    endif()
endif()

#----------------------------------------------------------------------
# Component 'gdal' (alias 'ogr')
if(Osmium_USE_GDAL)
    find_package(GDAL)

    list(APPEND OSMIUM_EXTRA_FIND_VARS GDAL_FOUND)
    if(GDAL_FOUND)
        list(APPEND OSMIUM_LIBRARIES ${GDAL_LIBRARIES})
        list(APPEND OSMIUM_INCLUDE_DIRS ${GDAL_INCLUDE_DIRS})
    else()
        message(WARNING "Osmium: GDAL library is required but not found, please install it or configure the paths.")
    endif()
endif()

#----------------------------------------------------------------------
# Component 'proj'
if(Osmium_USE_PROJ)
    find_path(PROJ_INCLUDE_DIR proj_api.h)
    find_library(PROJ_LIBRARY NAMES proj)

    list(APPEND OSMIUM_EXTRA_FIND_VARS PROJ_INCLUDE_DIR PROJ_LIBRARY)
    if(PROJ_INCLUDE_DIR AND PROJ_LIBRARY)
        set(PROJ_FOUND 1)
        list(APPEND OSMIUM_LIBRARIES ${PROJ_LIBRARY})
        list(APPEND OSMIUM_INCLUDE_DIRS ${PROJ_INCLUDE_DIR})
    else()
        message(WARNING "Osmium: PROJ.4 library is required but not found, please install it or configure the paths.")
    endif()
endif()

#----------------------------------------------------------------------
# Component 'sparsehash'
if(Osmium_USE_SPARSEHASH)
    find_path(SPARSEHASH_INCLUDE_DIR google/sparsetable)

    list(APPEND OSMIUM_EXTRA_FIND_VARS SPARSEHASH_INCLUDE_DIR)
    if(SPARSEHASH_INCLUDE_DIR)
        # Find size of sparsetable::size_type. This does not work on older
        # CMake versions because they can do this check only in C, not in C++.
        if(NOT CMAKE_VERSION VERSION_LESS 3.0)
           include(CheckTypeSize)
           set(CMAKE_REQUIRED_INCLUDES ${SPARSEHASH_INCLUDE_DIR})
           set(CMAKE_EXTRA_INCLUDE_FILES "google/sparsetable")
           check_type_size("google::sparsetable<int>::size_type" SPARSETABLE_SIZE_TYPE LANGUAGE CXX)
           set(CMAKE_EXTRA_INCLUDE_FILES)
           set(CMAKE_REQUIRED_INCLUDES)
        else()
           set(SPARSETABLE_SIZE_TYPE ${CMAKE_SIZEOF_VOID_P})
        endif()

        # Sparsetable::size_type must be at least 8 bytes (64bit), otherwise
        # OSM object IDs will not fit.
        if(SPARSETABLE_SIZE_TYPE GREATER 7)
            set(SPARSEHASH_FOUND 1)
            add_definitions(-DOSMIUM_WITH_SPARSEHASH=${SPARSEHASH_FOUND})
            list(APPEND OSMIUM_INCLUDE_DIRS ${SPARSEHASH_INCLUDE_DIR})
        else()
            message(WARNING "Osmium: Disabled Google SparseHash library on 32bit system (size_type=${SPARSETABLE_SIZE_TYPE}).")
        endif()
    else()
        message(WARNING "Osmium: Google SparseHash library is required but not found, please install it or configure the paths.")
    endif()
endif()

#----------------------------------------------------------------------

list(REMOVE_DUPLICATES OSMIUM_INCLUDE_DIRS)

if(OSMIUM_XML_LIBRARIES)
    list(REMOVE_DUPLICATES OSMIUM_XML_LIBRARIES)
endif()

if(OSMIUM_PBF_LIBRARIES)
    list(REMOVE_DUPLICATES OSMIUM_PBF_LIBRARIES)
endif()

if(OSMIUM_IO_LIBRARIES)
    list(REMOVE_DUPLICATES OSMIUM_IO_LIBRARIES)
endif()

if(OSMIUM_LIBRARIES)
    list(REMOVE_DUPLICATES OSMIUM_LIBRARIES)
endif()

#----------------------------------------------------------------------
#
#  Check that all required libraries are available
#
#----------------------------------------------------------------------
if(OSMIUM_EXTRA_FIND_VARS)
    list(REMOVE_DUPLICATES OSMIUM_EXTRA_FIND_VARS)
endif()
# Handle the QUIETLY and REQUIRED arguments and the optional version check
# and set OSMIUM_FOUND to TRUE if all listed variables are TRUE.
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Osmium
                                  REQUIRED_VARS OSMIUM_INCLUDE_DIR ${OSMIUM_EXTRA_FIND_VARS}
                                  VERSION_VAR _libosmium_version)
unset(OSMIUM_EXTRA_FIND_VARS)

#----------------------------------------------------------------------
#
#  A function for setting the -pthread option in compilers/linkers
#
#----------------------------------------------------------------------
function(set_pthread_on_target _target)
    if(NOT MSVC)
        set_target_properties(${_target} PROPERTIES COMPILE_FLAGS "-pthread")
        if(NOT APPLE)
            set_target_properties(${_target} PROPERTIES LINK_FLAGS "-pthread")
        endif()
    endif()
endfunction()

#----------------------------------------------------------------------
#
#  Add compiler flags
#
#----------------------------------------------------------------------
add_definitions(-D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64)

if(MSVC)
    add_definitions(-wd4996)

    # Disable warning C4068: "unknown pragma" because we want it to ignore
    # pragmas for other compilers.
    add_definitions(-wd4068)

    # Disable warning C4715: "not all control paths return a value" because
    # it generates too many false positives.
    add_definitions(-wd4715)

    # Disable warning C4351: new behavior: elements of array '...' will be
    # default initialized. The new behaviour is correct and we don't support
    # old compilers anyway.
    add_definitions(-wd4351)

    # Disable warning C4503: "decorated name length exceeded, name was truncated"
    # there are more than 150 of generated names in libosmium longer than 4096 symbols supported in MSVC
    add_definitions(-wd4503)

    add_definitions(-DNOMINMAX -DWIN32_LEAN_AND_MEAN -D_CRT_SECURE_NO_WARNINGS)
endif()

if(APPLE)
# following only available from cmake 2.8.12:
#   add_compile_options(-stdlib=libc++)
# so using this instead:
    add_definitions(-stdlib=libc++)
    set(LDFLAGS ${LDFLAGS} -stdlib=libc++)
endif()

#----------------------------------------------------------------------

# This is a set of recommended warning options that can be added when compiling
# libosmium code.
if(MSVC)
    set(OSMIUM_WARNING_OPTIONS "/W3 /wd4514" CACHE STRING "Recommended warning options for libosmium")
else()
    set(OSMIUM_WARNING_OPTIONS "-Wall -Wextra -pedantic -Wredundant-decls -Wdisabled-optimization -Wctor-dtor-privacy -Wnon-virtual-dtor -Woverloaded-virtual -Wsign-promo -Wold-style-cast" CACHE STRING "Recommended warning options for libosmium")
endif()

set(OSMIUM_DRACONIC_CLANG_OPTIONS "-Wdocumentation -Wunused-exception-parameter -Wmissing-declarations -Weverything -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-unused-macros -Wno-exit-time-destructors -Wno-global-constructors -Wno-padded -Wno-switch-enum -Wno-missing-prototypes -Wno-weak-vtables -Wno-cast-align -Wno-float-equal")

if(Osmium_DEBUG)
    message(STATUS "OSMIUM_XML_LIBRARIES=${OSMIUM_XML_LIBRARIES}")
    message(STATUS "OSMIUM_PBF_LIBRARIES=${OSMIUM_PBF_LIBRARIES}")
    message(STATUS "OSMIUM_IO_LIBRARIES=${OSMIUM_IO_LIBRARIES}")
    message(STATUS "OSMIUM_LIBRARIES=${OSMIUM_LIBRARIES}")
    message(STATUS "OSMIUM_INCLUDE_DIRS=${OSMIUM_INCLUDE_DIRS}")
endif()

