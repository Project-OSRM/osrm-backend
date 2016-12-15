# Locate V8
# This module defines
#  V8_FOUND, if false, do not try to link to V8
#  V8_LIBRARIES
#  V8_INCLUDE_DIR, where to find the V8 headers

find_path(V8_INCLUDE_DIR v8.h
    HINTS
        ENV V8_DIR
    PATHS
    ~/Library/Frameworks
    /Library/Frameworks
)

find_library(V8_BASE_LIBRARY
    NAMES v8_base
    HINTS
        ENV V8_DIR
    PATHS
    ~/Library/Frameworks
    /Library/Frameworks
)

find_library(V8_LIBBASE_LIBRARY
    NAMES v8_libbase
    HINTS
        ENV V8_DIR
    PATHS
    ~/Library/Frameworks
    /Library/Frameworks
)

find_library(V8_LIBPLATFORM_LIBRARY
    NAMES v8_libplatform
    HINTS
        ENV V8_DIR
    PATHS
    ~/Library/Frameworks
    /Library/Frameworks
)

find_library(V8_SNAPSHOT_LIBRARY
    NAMES v8_snapshot
    HINTS
        ENV V8_DIR
    PATHS
    ~/Library/Frameworks
    /Library/Frameworks
)

set(V8_FOUND "NO")

if(V8_INCLUDE_DIR AND V8_BASE_LIBRARY AND V8_LIBBASE_LIBRARY AND V8_LIBPLATFORM_LIBRARY AND V8_SNAPSHOT_LIBRARY)
    set(V8_FOUND "YES")
    set(V8_LIBRARIES ${V8_BASE_LIBRARY} ${V8_LIBBASE_LIBRARY} ${V8_LIBPLATFORM_LIBRARY} ${V8_SNAPSHOT_LIBRARY})
endif()

if (NOT V8_FOUND)
    if (V8_FIND_REQUIRED)
        message(FATAL_ERROR "Could NOT find v8 library and headers")
    endif (V8_FIND_REQUIRED)
endif
