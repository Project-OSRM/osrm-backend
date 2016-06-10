# - Try to find LibOSRM
# Once done this will define
#  LibOSRM_FOUND - System has LibOSRM
#  LibOSRM_LIBRARIES - The libraries and ldflags needed to use LibOSRM
#  LibOSRM_DEPENDENT_LIBRARIES - The libraries and ldflags need to link LibOSRM dependencies
#  LibOSRM_LIBRARY_DIRS - The libraries paths needed to find LibOSRM
#  LibOSRM_CXXFLAGS - Compiler switches required for using LibOSRM

find_package(PkgConfig)
pkg_search_module(PC_LibOSRM QUIET libosrm)

function(JOIN VALUES GLUE OUTPUT)
  string (REPLACE ";" "${GLUE}" _TMP_STR "${VALUES}")
  set (${OUTPUT} "${_TMP_STR}" PARENT_SCOPE)
endfunction()

list(REMOVE_ITEM PC_LibOSRM_CFLAGS " ")
JOIN("${PC_LibOSRM_CFLAGS}" " " output)

set(LibOSRM_CXXFLAGS ${output})
set(LibOSRM_LIBRARY_DIRS ${PC_LibOSRM_LIBRARY_DIRS})

find_path(LibOSRM_INCLUDE_DIR osrm/osrm.hpp
  PATH_SUFFIXES osrm include/osrm include
  HINTS ${PC_LibOSRM_INCLUDEDIR} ${PC_LibOSRM_INCLUDE_DIRS}
  ~/Library/Frameworks
  /Library/Frameworks
  /usr/local
  /usr
  /opt/local
  /opt)

find_library(TEST_LibOSRM_STATIC_LIBRARY Names osrm.lib libosrm.a
  PATH_SUFFIXES osrm lib/osrm lib
  HINTS ${PC_LibOSRM_LIBDIR} ${PC_LibOSRM_LIBRARY_DIRS}
  ~/Library/Frameworks
  /Library/Frameworks
  /usr/local
  /usr
  /opt/local
  /opt)
find_library(TEST_LibOSRM_DYNAMIC_LIBRARY Names libosrm.dylib libosrm.so
  PATH_SUFFIXES osrm lib/osrm lib
  HINTS ${PC_LibOSRM_LIBDIR} ${PC_LibOSRM_LIBRARY_DIRS}
  ~/Library/Frameworks
  /Library/Frameworks
  /usr/local
  /usr
  /opt/local
  /opt)

set(LibOSRM_DEPENDENT_LIBRARIES ${PC_LibOSRM_STATIC_LDFLAGS})
set(LibOSRM_LIBRARIES ${PC_LibOSRM_LDFLAGS})

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set LIBOSRM_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(LibOSRM DEFAULT_MSG
                                LibOSRM_LIBRARY_DIRS
                                LibOSRM_CXXFLAGS
                                LibOSRM_LIBRARIES
                                LibOSRM_DEPENDENT_LIBRARIES
                                LibOSRM_INCLUDE_DIR)
