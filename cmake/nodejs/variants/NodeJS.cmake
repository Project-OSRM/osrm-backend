set(NodeJS_URL_BASE http://nodejs.org/dist)
set(NodeJS_DEFAULT_VARIANT_BASE "node")
set(NodeJS_DEFAULT_WIN32_BINARY_NAME "${NodeJS_DEFAULT_VARIANT_BASE}.exe")
list(APPEND NodeJS_WIN32_DELAYLOAD ${NodeJS_DEFAULT_WIN32_BINARY_NAME})

if(NodeJS_VERSION STREQUAL "latest")
    set(NodeJS_LATEST_RELEASE_URL 
        "${NodeJS_URL_BASE}/latest/SHASUMS256.txt")
    nodejs_get_version(${NodeJS_LATEST_RELEASE_URL} NodeJS_VERSION)
endif()

if(NOT NodeJS_VARIANT_NAME)
    set(NodeJS_VARIANT_NAME ${NodeJS_DEFAULT_VARIANT_NAME})
endif()
if(NOT NodeJS_VARIANT_BASE)
    set(NodeJS_VARIANT_BASE ${NodeJS_DEFAULT_VARIANT_BASE})
endif()
if(NOT NodeJS_URL)
    set(NodeJS_URL "${NodeJS_URL_BASE}/v${NodeJS_VERSION}")
endif()

if(NOT NodeJS_SOURCE_PATH)
    set(NodeJS_SOURCE_PATH "${NodeJS_VARIANT_BASE}-v${NodeJS_VERSION}")
    # Use the headers archive when its available
    if(NodeJS_VERSION VERSION_GREATER ${NodeJS_HEADER_VERSION})
        set(NodeJS_SOURCE_PATH "${NodeJS_SOURCE_PATH}-headers")
    endif()
    set(NodeJS_SOURCE_PATH "${NodeJS_SOURCE_PATH}.tar.gz")
endif()

if(NodeJS_DEFAULT_INCLUDE AND 
    NodeJS_VERSION VERSION_GREATER ${NodeJS_HEADER_VERSION})
    set(NodeJS_SOURCE_INCLUDE False)
    set(NodeJS_HEADER_INCLUDE True)
endif()

if(NodeJS_SOURCE_INCLUDE)
    list(APPEND NodeJS_INCLUDE_PATHS
        src
        deps/uv/include
        deps/v8/include
        deps/zlib
    )
    # OpenSSL is an optional header
    if(NodeJS_HAS_OPENSSL)
        list(APPEND NodeJS_INCLUDE_PATHS
            deps/openssl/openssl/include
        )
    endif()
endif()
if(NodeJS_HEADER_INCLUDE)
    set(NodeJS_INCLUDE_PATHS include/node)
endif()

if(NOT NodeJS_CHECKSUM_TYPE)
    # Use SHA256 when available
    if(NodeJS_VERSION VERSION_GREATER ${NodeJS_SHA256_VERSION})
        set(NodeJS_CHECKSUM_TYPE "SHA256")
    else()
        set(NodeJS_CHECKSUM_TYPE "SHA1")
    endif()
endif()

if(NOT NodeJS_CHECKSUM_PATH)
    set(NodeJS_CHECKSUM_PATH "SHASUMS")
    if(NodeJS_CHECKSUM_TYPE STREQUAL "SHA256")
        set(NodeJS_CHECKSUM_PATH "${NodeJS_CHECKSUM_PATH}256")
    endif()
    set(NodeJS_CHECKSUM_PATH "${NodeJS_CHECKSUM_PATH}.txt")
endif()

# Library and binary are based on variant base
if(NOT NodeJS_WIN32_LIBRARY_NAME)
    set(NodeJS_WIN32_LIBRARY_NAME ${NodeJS_VARIANT_BASE}.lib)
endif()
if(NOT NodeJS_WIN32_BINARY_NAME)
    set(NodeJS_WIN32_BINARY_NAME ${NodeJS_VARIANT_BASE}.exe)
endif()

if(NOT NodeJS_WIN32_LIBRARY_PATH)
    # The library location is prefixed after a specific version
    if(NodeJS_HAS_WIN32_PREFIX AND 
       NodeJS_VERSION VERSION_GREATER ${NodeJS_PREFIX_VERSION})
        set(NodeJS_WIN32_LIBRARY_PATH "win-")
        if(NodeJS_ARCH_IA32)
            set(NodeJS_WIN32_LIBRARY_PATH "${NodeJS_WIN32_LIBRARY_PATH}x86/")
        endif()
    endif()
    # 64-bit versions are prefixed
    if(NodeJS_ARCH_X64)
        set(NodeJS_WIN32_LIBRARY_PATH "${NodeJS_WIN32_LIBRARY_PATH}x64/")
    endif()
    set(NodeJS_WIN32_LIBRARY_PATH 
        "${NodeJS_WIN32_LIBRARY_PATH}${NodeJS_WIN32_LIBRARY_NAME}"
    )
endif()

if(NodeJS_HAS_WIN32_BINARY AND NOT NodeJS_WIN32_BINARY_PATH)
    # The executable location is prefixed after a specific version
    if(NodeJS_HAS_WIN32_PREFIX AND 
       NodeJS_VERSION VERSION_GREATER ${NodeJS_PREFIX_VERSION})
        set(NodeJS_WIN32_BINARY_PATH "win-")
        if(NodeJS_ARCH_IA32)
            set(NodeJS_WIN32_BINARY_PATH "${NodeJS_WIN32_BINARY_PATH}x86/")
        endif()
    endif()
    # 64-bit versions are prefixed
    if(NodeJS_ARCH_X64)
        set(NodeJS_WIN32_BINARY_PATH "${NodeJS_WIN32_BINARY_PATH}x64/")
    endif()
    set(NodeJS_WIN32_BINARY_PATH 
        "${NodeJS_WIN32_BINARY_PATH}${NodeJS_WIN32_BINARY_NAME}"
    )
endif()

# Specify windows libraries
# XXX: This may need to be version/variant specific in the future
if(NodeJS_DEFAULT_LIBS AND NodeJS_PLATFORM_WIN32)
    list(APPEND NodeJS_LIBRARIES
        kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib
        advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib
        odbc32.lib DelayImp.lib
    )
endif()

mark_as_advanced(
    NodeJS_URL_BASE
    NodeJS_DEFAULT_VARIANT_BASE
    NodeJS_DEFAULT_WIN32_BINARY_NAME
    NodeJS_LATEST_RELEASE_URL
)