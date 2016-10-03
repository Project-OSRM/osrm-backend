# Locate Luabind library
# This module defines
#  LUABIND_FOUND, if false, do not try to link to Luabind
#  LUABIND_LIBRARIES
#  LUABIND_INCLUDE_DIR, where to find luabind.hpp

FIND_PACKAGE(Lua REQUIRED)
IF ("${LUA_VERSION_MAJOR}" STREQUAL "5" AND
   ("${LUA_VERSION_MINOR}" STREQUAL "1" OR "${LUA_VERSION_MINOR}" STREQUAL "2"))
	MESSAGE(STATUS "Using Lua ${LUA_VERSION_STRING}")
ELSE()
	MESSAGE(FATAL_ERROR "Lua ${LUA_VERSION_STRING} detected; need 5.1 or 5.2")
ENDIF()

FIND_PATH(LUABIND_INCLUDE_DIR luabind.hpp
  HINTS
  $ENV{LUABIND_DIR}
  PATH_SUFFIXES luabind include/luabind include
  PATHS
  ~/Library/Frameworks
  /Library/Frameworks
  /usr/local
  /usr
  /opt/local # DarwinPorts
  /opt
)

FIND_LIBRARY(LUABIND_LIBRARY
  NAMES luabind luabind09
  HINTS
  $ENV{LUABIND_DIR}
  PATH_SUFFIXES lib64 lib
  PATHS
  ~/Library/Frameworks
  /Library/Frameworks
  /usr/local
  /usr
  /opt/local
  /opt
)

FIND_LIBRARY(LUABIND_LIBRARY_DBG
  NAMES luabindd
  HINTS
  $ENV{LUABIND_DIR}
  PATH_SUFFIXES lib64 lib
  PATHS
  ~/Library/Frameworks
  /Library/Frameworks
  /usr/local
  /usr
  /opt/local
  /opt
)

IF(LUABIND_LIBRARY)
    SET( LUABIND_LIBRARIES "${LUABIND_LIBRARY}" CACHE STRING "Luabind Libraries")
ENDIF(LUABIND_LIBRARY)

INCLUDE(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set LUABIND_FOUND to TRUE if
# all listed variables are TRUE
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Luabind  DEFAULT_MSG  LUABIND_LIBRARIES LUABIND_INCLUDE_DIR)

IF( NOT LUABIND_FIND_QUIETLY )
    IF( LUABIND_FOUND )
        MESSAGE(STATUS "Found Luabind: ${LUABIND_LIBRARY}" )
    ENDIF()
    IF( LUABIND_LIBRARY_DBG )
        MESSAGE(STATUS "Luabind debug library availible: ${LUABIND_LIBRARY_DBG}")
    ENDIF()
ENDIF()

MARK_AS_ADVANCED(LUABIND_INCLUDE_DIR LUABIND_LIBRARIES LUABIND_LIBRARY LUABIND_LIBRARY_DBG)
