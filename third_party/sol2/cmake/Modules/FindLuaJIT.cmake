# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

#.rst:
# FindLuaJIT
# -------
#
#
#
# Locate LuaJIT library. This module defines
#
# ::
#
#   LUAJIT_FOUND                - if false, do not try to link to LuaJIT
#   LUAJIT_LIBRARIES            - both lua and lualib
#   LUAJIT_INCLUDE_DIR          - where to find lua.h and luajit.h
#   LUAJIT_VERSION_STRING       - the version of LuaJIT found
#   LUAJIT_VERSION_MAJOR        - the major version of LuaJIT
#   LUAJIT_VERSION_MINOR        - the minor version of LuaJIT
#   LUAJIT_VERSION_PATCH        - the patch version of LuaJIT
#   LUAJIT_LUA_VERSION_STRING   - the version of Lua the found LuaJIT is compatible with
#
#
#
# Note that the expected include convention is
#
# ::
#
#   #include "lua.h"
#
# and not
#
# ::
#
#   #include <lua/lua.h>
#
# This is because, the lua location is not standardized and may exist in
# locations other than lua/

unset(_luajit_include_subdirs)
unset(_luajit_append_versions)
unset(_luajit_library_names)

include(${CMAKE_CURRENT_LIST_DIR}/FindLua/set_version_vars.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/FindLua/version_check.cmake)

_lua_set_version_vars(luajit "jit")

find_path(LUAJIT_INCLUDE_DIR luajit.h
  HINTS
    ENV LUAJIT_DIR
  PATH_SUFFIXES ${_luajit_include_subdirs} include/luajit include
  PATHS
    ${LUAJIT_DIR}
    ~/Library/Frameworks
    /Library/Frameworks
    /sw # Fink
    /opt/local # DarwinPorts
    /opt/csw # Blastwave
    /opt
    /usr
    /usr/local # Homebrew
)

if (LUAJIT_INCLUDE_DIR AND EXISTS "${LUAJIT_INCLUDE_DIR}/lua.h")
    _lua_check_header_version("${LUAJIT_INCLUDE_DIR}/lua.h" "LUAJIT")
endif ()

if (NOT LUAJIT_VERSION_STRING)
    foreach (subdir IN LISTS _luajit_include_subdirs)
        unset(LUAJIT_INCLUDE_PREFIX CACHE)
        find_path(LUAJIT_INCLUDE_PREFIX ${subdir}/lua.h
          HINTS
            ENV LUA_DIR
          PATHS
            ${LUA_DIR}
            ~/Library/Frameworks
            /Library/Frameworks
            /sw # Fink
            /opt/local # DarwinPorts
            /opt/csw # Blastwave
            /opt
            /usr
            /usr/local # Homebrew
        )
        if (LUAJIT_INCLUDE_PREFIX)
            _lua_check_header_version("${LUAJIT_INCLUDE_PREFIX}/${subdir}/lua.h" "LUAJIT")
            if (LUAJIT_VERSION_STRING)
                set(LUAJIT_INCLUDE_DIR "${LUAJIT_INCLUDE_PREFIX}/${subdir}")
                break()
            endif ()
        endif ()
    endforeach ()
endif ()
unset(_luajit_include_subdirs)
unset(_luajit_append_versions)

if (LUAJIT_INCLUDE_DIR AND EXISTS "${LUAJIT_INCLUDE_DIR}/luajit.h")
    # LuaJIT defines two preprocessor macros:
    #   LUA_VERSION     -> version string with lua version in it
    #   LUA_VERSION_NUM -> numeric representation, i.e. 20003 for 2.0.3
    # This just parses the LUAJIT_VERSION macro and extracts the version.
    file(STRINGS "${LUAJIT_INCLUDE_DIR}/luajit.h" version_strings
         REGEX "^#define[ \t]+LUAJIT_VERSION?[ \t]+(\"LuaJIT [0-9\\.]+(-(beta|alpha)[0-9]*)?\").*")

    string(REGEX REPLACE ".*;#define[ \t]+LUAJIT_VERSION[ \t]+\"LuaJIT ([0-9\\.]+(-(beta|alpha)[0-9]*)?)\"[ \t]*;.*" "\\1" LUAJIT_VERSION_STRING_SHORT ";${version_strings};")
    string(REGEX REPLACE ".*;([0-9]+\\.[0-9]+\\.[0-9]+(-(beta|alpha)[0-9]*)?);.*" "\\1" luajit_version_num ";${LUAJIT_VERSION_STRING_SHORT};")

    string(REGEX REPLACE "^([0-9]+)\\.[0-9]+\\.(-(beta|alpha)[0-9]*)?$" "\\1" LUAJIT_VERSION_MAJOR "${luajit_version_num}")
    string(REGEX REPLACE "^[0-9]+\\.([0-9]+)\\.[0-9](-(beta|alpha)[0-9]*)?$" "\\1" LUAJIT_VERSION_MINOR "${luajit_version_num}")
    string(REGEX REPLACE "^[0-9]+\\.[0-9]+\\.([0-9]+(-(beta|alpha)[0-9]*)?)$" "\\1" LUAJIT_VERSION_PATCH "${luajit_version_num}")

    # We can also set the LUAJIT_LUA_VERSION_* fields that are found by FindLua.
    # We do this as LuaJIT claims full compatibility with a certain LUA version.
    _lua_check_header_version("${LUAJIT_INCLUDE_DIR}/lua.h" "LUAJIT_LUA_")

    set(LUAJIT_VERSION_STRING "${LUAJIT_LUA_VERSION_STRING} (${LUAJIT_VERSION_STRING_SHORT})")
endif()

find_library(LUAJIT_LIBRARY
  NAMES ${_luajit_library_names} luajit lua
  HINTS
    ENV LUAJIT_DIR
  PATH_SUFFIXES lib
  PATHS
    ${LUAJIT_DIR}
    ~/Library/Frameworks
    /Library/Frameworks
    /sw
    /opt/local
    /opt/csw
    /opt
    /usr
    /usr/local # Homebrew
)
unset(_luajit_library_names)

if (LUAJIT_LIBRARY)
    # include the math library for Unix
    if (UNIX AND NOT APPLE AND NOT BEOS)
        find_library(LUAJIT_MATH_LIBRARY m)
        set(LUAJIT_LIBRARIES "${LUAJIT_LIBRARY};${LUAJIT_MATH_LIBRARY}")
    # For Windows and Mac, don't need to explicitly include the math library
    else ()
        set(LUAJIT_LIBRARIES "${LUAJIT_LIBRARY}")
    endif ()

    set(LUAJIT_LIBRARY_DIR )
    foreach (lib ${LUAJIT_LIBRARIES})
        get_filename_component(lib_dir ${lib} DIRECTORY CACHE)
        list(APPEND LUAJIT_LIBRARY_DIR ${lib_dir})
    endforeach ()
    list(REMOVE_DUPLICATES LUAJIT_LIBRARY_DIR)
endif ()

if(APPLE)
    # Not sure why this is true, but its mentioned here:
    # http://luajit.org/install.html#embed
    set(LUAJIT_LINK_FLAGS "-pagezero_size 10000 -image_base 100000000")
else()
    set(LUAJIT_LINK_FLAGS "")
endif()

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set LuaJIT_FOUND to TRUE if
# all listed variables are TRUE
FIND_PACKAGE_HANDLE_STANDARD_ARGS(LuaJIT
                                  FOUND_VAR LuaJIT_FOUND
                                  REQUIRED_VARS LUAJIT_LIBRARIES LUAJIT_INCLUDE_DIR LUAJIT_LIBRARY_DIR
                                  VERSION_VAR LUAJIT_VERSION_STRING)

mark_as_advanced(LUAJIT_INCLUDE_DIR LUAJIT_LIBRARY LUAJIT_LIBRARY_DIR LUAJIT_MATH_LIBRARY LUAJIT_LINK_FLAGS)

