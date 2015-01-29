# - Try to find Libgomp
# Once done this will define
#  LIBGOMP_FOUND - System has Libgomp
#  LIBGOMP_INCLUDE_DIRS - The Libgomp include directories
#  LIBGOMP_DEFINITIONS - Compiler switches required for using Libgomp

message(STATUS "Checking if libgomp is present")

find_library(LIBGOMP_LIBRARY NAMES gomp libgomp
             HINTS ${PC_LIBXML_LIBDIR} ${PC_LIBXML_LIBRARY_DIRS} )

set(LIBGOMP_LIBRARIES ${LIBGOMP_LIBRARY} )

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set LIBGOMP_FOUND to TRUE
# if all listed variables are TRUE
#find_package_handle_standard_args(Libgomp  DEFAULT_MSG
#                                  LIBGOMP_LIBRARY LIBGOMP_INCLUDE_DIR)
