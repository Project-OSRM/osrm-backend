set(NWJS_URL_BASE "http://dl.nwjs.io")
set(NWJS_VARIANT_BASE "nw")
set(NWJS_WIN32_BINARY_NAME  "${NWJS_VARIANT_BASE}.exe")
list(APPEND NodeJS_WIN32_DELAYLOAD ${NWJS_WIN32_BINARY_NAME})

if(NodeJS_FIND_REQUIRED_NWJS OR NodeJS_VARIANT STREQUAL ${NWJS_VARIANT_BASE})
    set(NodeJS_CHECKSUM_PATH "MD5SUMS")
    set(NodeJS_CHECKSUM_TYPE "MD5")
    
    if(NodeJS_VERSION STREQUAL "latest")
        set(NWJS_LATEST_RELEASE_URL 
            "${NWJS_URL_BASE}/latest/${NodeJS_CHECKSUM_PATH}")
        nodejs_get_version(${NWJS_LATEST_RELEASE_URL} NodeJS_VERSION)
    endif()

    set(NodeJS_VARIANT_NAME "nw.js")
    set(NodeJS_VARIANT_BASE ${NWJS_VARIANT_BASE})
    set(NodeJS_URL "${NWJS_URL_BASE}/v${NodeJS_VERSION}")
    set(NodeJS_SOURCE_PATH "nw-headers-v${NodeJS_VERSION}.tar.gz")
    set(NodeJS_DEFAULT_INCLUDE False)
    set(NodeJS_HAS_WIN32_PREFIX False)
    set(NodeJS_HAS_WIN32_BINARY False)
endif()

mark_as_advanced(
    NWJS_URL_BASE
    NWJS_VARIANT_BASE
    NWJS_WIN32_BINARY_NAME
    NWJS_LATEST_RELEASE_URL
)