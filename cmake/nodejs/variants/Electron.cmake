set(ELECTRON_VARIANT_BASE "electron")
set(ELECTRON_WIN32_BINARY_NAME "${ELECTRON_VARIANT_BASE}.exe")
list(APPEND NodeJS_WIN32_DELAYLOAD ${ELECTRON_WIN32_BINARY_NAME})

if(NodeJS_FIND_REQUIRED_ELECTRON OR 
   NodeJS_VARIANT STREQUAL ${ELECTRON_VARIANT_BASE})
    if(NodeJS_VERSION STREQUAL "latest")
        include(util/Github)
        github_get_rate_limit(GITHUB_RATE_LIMIT)

        # Handle determining the latest release
        # Very complicated, due to electron not following the "latest"
        # convention of other variants
        set(ELECTRON_LATEST_RELEASE_FILE ${CMAKE_CURRENT_BINARY_DIR}/ELECTRON)
        set(ELECTRON_LATEST_RELEASE_URL
            ${GITHUB_API_URL}/repos/atom/electron/releases/latest${GITHUB_AUTH}
        )
        if(GITHUB_RATE_LIMIT GREATER 0)
            nodejs_download(
                ${ELECTRON_LATEST_RELEASE_URL}
                ${ELECTRON_LATEST_RELEASE_FILE}
                ON
            )
        endif()
        nodejs_check_file(
            ${ELECTRON_LATEST_RELEASE_FILE} 
            "Releases file could not be downloaded, likely \
            because github rate limit was exceeded. Wait until the limit \
            passes or set GITHUB_API_TOKEN in your environment to a valid \
            github developer token."
        )
        file(READ ${ELECTRON_LATEST_RELEASE_FILE} ELECTRON_LATEST_RELEASE_DATA)
            string(REGEX MATCH "\"tag_name\"\: \"v([0-9]+\.[0-9]+\.[0-9]+)\"" 
        ELECTRON_LATEST_RELEASE_MATCH ${ELECTRON_LATEST_RELEASE_DATA})
        set(NodeJS_VERSION ${CMAKE_MATCH_1})
    endif()

    set(NodeJS_VARIANT_NAME "Electron.js")

    # SHASUMS of any kind is inaccessible prior to 0.16.0
    if(NodeJS_VERSION VERSION_LESS 0.16.0)
        message(FATAL_ERROR "Electron is only supported for versions >= 0.16.0")
    endif()
    
    # Electron switched to IOJS after 0.25.0
    # Probably needs to be bounded on the upper side if/when they switch
    # back to node mainline due to iojs-node merge
    set(NodeJS_VARIANT_BASE "node")
    if(NodeJS_VERSION VERSION_GREATER 0.25.0)
        set(NodeJS_VARIANT_BASE "iojs")
    endif()

    # Url is hard to get, because it will immediately resolve to a CDN
    # Extracted from the electron website
    set(NodeJS_URL 
        "https://atom.io/download/atom-shell/v${NodeJS_VERSION}"
    )

    # Headers become available for IOJS base ONLY!
    # Variant base switch above handles this
    set(NodeJS_HEADER_VERSION 0.30.1)

    # Header only archive uses source style paths
    set(NodeJS_DEFAULT_INCLUDE False)

    # Hard to determine, but versions seem to start at 16, and SHA256 is
    # available
    set(NodeJS_SHA256_VERSION 0.15.9)

    # C++11 and Prefixing start after the IOJS switch
    # Will carry forward after node mainline so no need for upper bound (whew)
    set(NodeJS_PREFIX_VERSION 0.25.0)
    set(NodeJS_CXX11R_VERSION 0.25.0)

    # The executable is not provided on the CDN
    # In theory, I could support a BINARY_URL to get this from github
    set(NodeJS_HAS_WIN32_BINARY False)

    # OpenSSL isn't included in the headers
    set(NodeJS_HAS_OPENSSL False)
endif()