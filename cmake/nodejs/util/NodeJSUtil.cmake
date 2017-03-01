function(nodejs_check_file FILE)
    set(MESSAGE "File ${FILE} does not exist or is empty")
    if(ARGC GREATER 1)
        set(MESSAGE ${ARGV1})
    endif()

    # Make sure the file has contents
    file(READ ${FILE} FILE_CONTENT LIMIT 1 HEX)
    if(NOT FILE_CONTENT)
        file(REMOVE ${FILE})
        message(FATAL_ERROR ${MESSAGE})
    endif()
endfunction()

function(nodejs_download URL FILE)
    # Function optionally takes a checksum and a checksum type, and
    # a force value
    # Either can be specified without the other, but checksum must come first
    if(ARGC GREATER 2)
        set(CHECKSUM ${ARGV2})
        if(CHECKSUM STREQUAL "On" OR CHECKSUM STREQUAL "ON" OR
            CHECKSUM STREQUAL "True" OR CHECKSUM STREQUAL "TRUE" OR
            CHECKSUM STREQUAL "Off" OR CHECKSUM STREQUAL "OFF" OR
            CHECKSUM STREQUAL "False" OR CHECKSUM STREQUAL "FALSE")
            set(FORCE ${CHECKSUM})
            unset(CHECKSUM)
        elseif(ARGC GREATER 3)
            set(TYPE ${ARGV3})
        else()
            message(FATAL_ERROR "Checksum type must be specified")
        endif()
    elseif(ARGC GREATER 4)
        set(CHECKSUM ${ARGV2})
        set(TYPE ${ARGV3})
        set(FORCE ${ARGV4})
    endif()

    # If the file exists, no need to download it again unless its being forced
    if(NOT FORCE AND EXISTS ${FILE})
        return()
    endif()

    # Download the file
    message(STATUS "Downloading: ${URL}")
    file(DOWNLOAD 
        ${URL}
        ${FILE}
        SHOW_PROGRESS
    )

    # Make sure the file has contents
    nodejs_check_file(${FILE} "Unable to download ${URL}")

    # If a checksum is provided, validate the downloaded file
    if(CHECKSUM)
        message(STATUS "Validating: ${FILE}")
        file(${TYPE} ${FILE} DOWNLOAD_CHECKSUM)
        message(STATUS "Checksum: ${CHECKSUM}")
        message(STATUS "Download: ${DOWNLOAD_CHECKSUM}")
        if(NOT CHECKSUM STREQUAL DOWNLOAD_CHECKSUM)
            file(REMOVE ${FILE})
            message(FATAL_ERROR "Validation failure: ${FILE}")
        endif()
    endif()
endfunction()

function(nodejs_checksum DATA FILE VAR)
    string(REGEX MATCH "([A-Fa-f0-9]+)[\t ]+${FILE}" CHECKSUM_MATCH ${DATA})
    if(CMAKE_MATCH_1)
        set(${VAR} ${CMAKE_MATCH_1} PARENT_SCOPE)
    else()
        message(FATAL_ERROR "Unable to extract file checksum")
    endif()
endfunction()

function(nodejs_extract FILE DIR)
    # Function optionally takes a force value
    if(ARGC GREATER 2)
        set(FORCE ${ARGV2})
    endif()

    # If the archvie has been extracted, no need to extract again unless it
    # is being forced
    if(NOT FORCE AND EXISTS ${DIR})
        return()
    endif()

    # Make a temporary directory for extracting the output
    set(EXTRACT_DIR ${CMAKE_CURRENT_BINARY_DIR}/extract)
    if(EXISTS ${EXTRACT_DIR})
        file(REMOVE_RECURSE ${EXTRACT_DIR})
    endif()
    file(MAKE_DIRECTORY ${EXTRACT_DIR})

    # Extract the archive
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E tar xfz ${FILE}
        WORKING_DIRECTORY ${EXTRACT_DIR}
    )
    
    # If only one element is extracted, the archive contained a nested
    # folder; use the inner folder as the extracted folder
    file(GLOB EXTRACT_CHILDREN ${EXTRACT_DIR}/*)
    list(LENGTH EXTRACT_CHILDREN NUM_CHILDREN)
    set(TARGET_DIR ${EXTRACT_DIR})
    if(NUM_CHILDREN EQUAL 1)
        list(GET EXTRACT_CHILDREN 0 TARGET_DIR)
    endif()

    # Move the folder to the target path
    if(EXISTS ${DIR})
        file(REMOVE_RECURSE ${DIR})
    endif()
    file(RENAME ${TARGET_DIR} ${DIR})
    
    # Make sure to clean up the extraction folder when the inner folder
    # is used
    file(REMOVE_RECURSE ${EXTRACT_DIR})
endfunction()

function(nodejs_find_module NAME BASE PATH)
    # Find a node module using the same search path that require uses
    # without needing a node binary
    set(ROOT ${BASE})
    set(DRIVE "^[A-Za-z]?:?/$")

    # Walk up the directory tree until at the root
    while(NOT ROOT MATCHES ${DRIVE} AND NOT 
        EXISTS ${ROOT}/node_modules/${NAME})
        get_filename_component(ROOT ${ROOT} DIRECTORY)
    endwhile()

    # Operate like the CMake find_* functions, returning NOTFOUND if the
    # module can't be found
    if(ROOT MATCHES ${DRIVE})
        set(${PATH} ${NAME}-NOTFOUND PARENT_SCOPE)
    else()
        set(${PATH} ${ROOT}/node_modules/${NAME} PARENT_SCOPE)
    endif()
endfunction()

macro(nodejs_find_module_fallback NAME BASE PATH)
    # Look in the provided path first
    # If the module isn't found, try searching from the module
    nodejs_find_module(${NAME} ${BASE} ${PATH})
    if(NOT ${PATH})
        nodejs_find_module(${NAME} ${NodeJS_MODULE_PATH} ${PATH})
    endif()
endmacro()

function(nodejs_get_version URL VAR)
    set(NWJS_LATEST_RELEASE_URL 
        "${NWJS_URL_BASE}/latest/${NodeJS_CHECKSUM_PATH}")
    set(VERSION_FILE ${CMAKE_CURRENT_BINARY_DIR}/VERSION)
    nodejs_download(
        ${URL}
        ${VERSION_FILE}
        ON
    )
    nodejs_check_file(${VERSION_FILE})
    file(READ ${VERSION_FILE} VERSION_DATA)
    string(REGEX MATCH "v([0-9]+\.[0-9]+\.[0-9]+)" 
        VERSION_MATCH ${VERSION_DATA}
    )
    set(${VAR} ${CMAKE_MATCH_1} PARENT_SCOPE)
endfunction()