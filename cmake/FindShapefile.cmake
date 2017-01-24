# - Try to find Shapefile C Library
#   http://shapelib.maptools.org/
#
# Exports:
#  Shapefile_FOUND
#  LIBSHAPEFILE_INCLUDE_DIR
#  LIBSHAPEFILE_LIBRARY
# Hints:
#  LIBSHAPEFILE_LIBRARY_DIR

find_path(LIBSHAPEFILE_INCLUDE_DIR
          shapefil.h)

find_library(LIBSHAPEFILE_LIBRARY
             NAMES shp
             HINTS "${LIBSHAPEFILE_LIBRARY_DIR}")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Shapefile DEFAULT_MSG
                                  LIBSHAPEFILE_LIBRARY LIBSHAPEFILE_INCLUDE_DIR)
mark_as_advanced(LIBSHAPEFILE_INCLUDE_DIR LIBSHAPEFILE_LIBRARY)
