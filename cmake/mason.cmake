# Mason CMake

include(CMakeParseArguments)

function(mason_detect_platform)
    # Determine platform
    if(NOT MASON_PLATFORM)
        # we call uname -s manually here since
        # CMAKE_HOST_SYSTEM_NAME will not be defined before the project() call
        execute_process(
            COMMAND uname -s
            OUTPUT_VARIABLE UNAME
            OUTPUT_STRIP_TRAILING_WHITESPACE)

        if (UNAME STREQUAL "Darwin")
            set(MASON_PLATFORM "osx" PARENT_SCOPE)
        else()
            set(MASON_PLATFORM "linux" PARENT_SCOPE)
        endif()
    endif()

    # Determine platform version string
    if(NOT MASON_PLATFORM_VERSION)
        execute_process(
            COMMAND uname -m
            OUTPUT_VARIABLE MASON_PLATFORM_VERSION
            OUTPUT_STRIP_TRAILING_WHITESPACE)
        set(MASON_PLATFORM_VERSION "${MASON_PLATFORM_VERSION}" PARENT_SCOPE)
    endif()
endfunction()

function(mason_use _PACKAGE)
    if(NOT _PACKAGE)
        message(FATAL_ERROR "[Mason] No package name given")
    endif()

    cmake_parse_arguments("" "HEADER_ONLY" "VERSION" "" ${ARGN})

    if(_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "[Mason] mason_use() called with unrecognized arguments: ${_UNPARSED_ARGUMENTS}")
    endif()

    if(NOT _VERSION)
        message(FATAL_ERROR "[Mason] Specifying a version is required")
    endif()

    if(MASON_PACKAGE_${_PACKAGE}_INVOCATION STREQUAL "${MASON_INVOCATION}")
        # Check that the previous invocation of mason_use didn't select another version of this package
        if(NOT MASON_PACKAGE_${_PACKAGE}_VERSION STREQUAL ${_VERSION})
            message(FATAL_ERROR "[Mason] Already using ${_PACKAGE} ${MASON_PACKAGE_${_PACKAGE}_VERSION}. Cannot select version ${_VERSION}.")
        endif()
    else()
        if(_HEADER_ONLY)
            set(_PLATFORM_ID "headers")
        else()
            set(_PLATFORM_ID "${MASON_PLATFORM}-${MASON_PLATFORM_VERSION}")
        endif()

        set(_SLUG "${_PLATFORM_ID}/${_PACKAGE}/${_VERSION}")
        set(_INSTALL_PATH "${MASON_PACKAGE_DIR}/${_SLUG}")
        file(RELATIVE_PATH _INSTALL_PATH_RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "${_INSTALL_PATH}")

        if(NOT EXISTS "${_INSTALL_PATH}")
            set(_CACHE_PATH "${MASON_PACKAGE_DIR}/.binaries/${_SLUG}.tar.gz")
            if (NOT EXISTS "${_CACHE_PATH}")
                # Download the package
                set(_URL "${MASON_REPOSITORY}/${_SLUG}.tar.gz")
                message("[Mason] Downloading package ${_URL}...")

                set(_FAILED)
                set(_ERROR)
                # Note: some CMake versions are compiled without SSL support
                get_filename_component(_CACHE_DIR "${_CACHE_PATH}" DIRECTORY)
                file(MAKE_DIRECTORY "${_CACHE_DIR}")
                execute_process(
                    COMMAND curl --retry 3 -s -f -S -L "${_URL}" -o "${_CACHE_PATH}.tmp"
                    RESULT_VARIABLE _FAILED
                    ERROR_VARIABLE _ERROR)
                if(_FAILED)
                    message(FATAL_ERROR "[Mason] Failed to download ${_URL}: ${_ERROR}")
                else()
                    # We downloaded to a temporary file to prevent half-finished downloads
                    file(RENAME "${_CACHE_PATH}.tmp" "${_CACHE_PATH}")
                endif()
            endif()

            # Unpack the package
            message("[Mason] Unpacking package to ${_INSTALL_PATH_RELATIVE}...")
            file(MAKE_DIRECTORY "${_INSTALL_PATH}")
            execute_process(
                COMMAND ${CMAKE_COMMAND} -E tar xzf "${_CACHE_PATH}"
                WORKING_DIRECTORY "${_INSTALL_PATH}")
        endif()

        # Error out if there is no config file.
        if(NOT EXISTS "${_INSTALL_PATH}/mason.ini")
            message(FATAL_ERROR "[Mason] Could not find mason.ini for package ${_PACKAGE} ${_VERSION}")
        endif()

        set(MASON_PACKAGE_${_PACKAGE}_PREFIX "${_INSTALL_PATH}" CACHE STRING "${_PACKAGE} ${_INSTALL_PATH}" FORCE)
        mark_as_advanced(MASON_PACKAGE_${_PACKAGE}_PREFIX)

        # Load the configuration from the ini file
        file(STRINGS "${_INSTALL_PATH}/mason.ini" _CONFIG_FILE)
        foreach(_LINE IN LISTS _CONFIG_FILE)
            string(REGEX MATCH "^([a-z_]+) *= *" _KEY "${_LINE}")
            if (_KEY)
                string(LENGTH "${_KEY}" _KEY_LENGTH)
                string(SUBSTRING "${_LINE}" ${_KEY_LENGTH} -1 _VALUE)
                string(REGEX REPLACE ";.*$" "" _VALUE "${_VALUE}") # Trim trailing commas
                string(REPLACE "{prefix}" "${_INSTALL_PATH}" _VALUE "${_VALUE}")
                string(STRIP "${_VALUE}" _VALUE)
                string(REPLACE "=" "" _KEY "${_KEY}")
                string(STRIP "${_KEY}" _KEY)
                string(TOUPPER "${_KEY}" _KEY)
                if(_KEY STREQUAL "INCLUDE_DIRS" OR _KEY STREQUAL "STATIC_LIBS" )
                    separate_arguments(_VALUE)
                endif()
                set(MASON_PACKAGE_${_PACKAGE}_${_KEY} "${_VALUE}" CACHE STRING "${_PACKAGE} ${_KEY}" FORCE)
                mark_as_advanced(MASON_PACKAGE_${_PACKAGE}_${_KEY})
            endif()
        endforeach()

        # Compare version in the package to catch errors early on
        if(NOT _VERSION STREQUAL MASON_PACKAGE_${_PACKAGE}_VERSION)
            message(FATAL_ERROR "[Mason] Package at ${_INSTALL_PATH_RELATIVE} has version '${MASON_PACKAGE_${_PACKAGE}_VERSION}', but required '${_VERSION}'")
        endif()

        if(NOT _PACKAGE STREQUAL MASON_PACKAGE_${_PACKAGE}_NAME)
            message(FATAL_ERROR "[Mason] Package at ${_INSTALL_PATH_RELATIVE} has name '${MASON_PACKAGE_${_PACKAGE}_NAME}', but required '${_NAME}'")
        endif()

        if(NOT _HEADER_ONLY)
            if(NOT MASON_PLATFORM STREQUAL MASON_PACKAGE_${_PACKAGE}_PLATFORM)
                message(FATAL_ERROR "[Mason] Package at ${_INSTALL_PATH_RELATIVE} has platform '${MASON_PACKAGE_${_PACKAGE}_PLATFORM}', but required '${MASON_PLATFORM}'")
            endif()

            if(NOT MASON_PLATFORM_VERSION STREQUAL MASON_PACKAGE_${_PACKAGE}_PLATFORM_VERSION)
                message(FATAL_ERROR "[Mason] Package at ${_INSTALL_PATH_RELATIVE} has platform version '${MASON_PACKAGE_${_PACKAGE}_PLATFORM_VERSION}', but required '${MASON_PLATFORM_VERSION}'")
            endif()
        endif()

        # Concatenate the static libs and libraries
        set(_LIBRARIES)
        list(APPEND _LIBRARIES ${MASON_PACKAGE_${_PACKAGE}_STATIC_LIBS} ${MASON_PACKAGE_${_PACKAGE}_LDFLAGS})
        set(MASON_PACKAGE_${_PACKAGE}_LIBRARIES "${_LIBRARIES}" CACHE STRING "${_PACKAGE} _LIBRARIES" FORCE)
        mark_as_advanced(MASON_PACKAGE_${_PACKAGE}_LIBRARIES)

        if(NOT _HEADER_ONLY)
            string(REGEX MATCHALL "(^| +)-L *([^ ]+)" MASON_PACKAGE_${_PACKAGE}_LIBRARY_DIRS "${MASON_PACKAGE_${_PACKAGE}_LDFLAGS}")
            string(REGEX REPLACE "(^| +)-L *" "\\1" MASON_PACKAGE_${_PACKAGE}_LIBRARY_DIRS "${MASON_PACKAGE_${_PACKAGE}_LIBRARY_DIRS}")
            set(MASON_PACKAGE_${_PACKAGE}_LIBRARY_DIRS "${MASON_PACKAGE_${_PACKAGE}_LIBRARY_DIRS}" CACHE STRING "${_PACKAGE} ${MASON_PACKAGE_${_PACKAGE}_LIBRARY_DIRS}" FORCE)
            mark_as_advanced(MASON_PACKAGE_${_PACKAGE}_LIBRARY_DIRS)
        endif()

        # Store invocation ID to prevent different versions of the same package in one invocation
        set(MASON_PACKAGE_${_PACKAGE}_INVOCATION "${MASON_INVOCATION}" CACHE INTERNAL "${_PACKAGE} invocation ID" FORCE)
    endif()
endfunction()

macro(target_add_mason_package _TARGET _VISIBILITY _PACKAGE)
    if (NOT MASON_PACKAGE_${_PACKAGE}_INVOCATION)
        message(FATAL_ERROR "[Mason] Package ${_PACKAGE} has not been initialized yet")
    endif()

    target_include_directories(${_TARGET} ${_VISIBILITY} "${MASON_PACKAGE_${_PACKAGE}_INCLUDE_DIRS}")
    target_compile_definitions(${_TARGET} ${_VISIBILITY} "${MASON_PACKAGE_${_PACKAGE}_DEFINITIONS}")
    target_compile_options(${_TARGET} ${_VISIBILITY} "${MASON_PACKAGE_${_PACKAGE}_OPTIONS}")
    target_link_libraries(${_TARGET} ${_VISIBILITY} "${MASON_PACKAGE_${_PACKAGE}_LIBRARIES}")
endmacro()

# Setup

string(RANDOM LENGTH 16 MASON_INVOCATION)

# Read environment variables if CMake is run in command mode
if (CMAKE_ARGC)
    set(MASON_PLATFORM "$ENV{MASON_PLATFORM}")
    set(MASON_PLATFORM_VERSION "$ENV{MASON_PLATFORM_VERSION}")
    set(MASON_PACKAGE_DIR "$ENV{MASON_PACKAGE_DIR}")
    set(MASON_REPOSITORY "$ENV{MASON_REPOSITORY}")
endif()

# Directory where Mason packages are located; typically ends with mason_packages
if (NOT MASON_PACKAGE_DIR)
    set(MASON_PACKAGE_DIR "${CMAKE_SOURCE_DIR}/mason_packages")
endif()

# URL prefix of where packages are located.
if (NOT MASON_REPOSITORY)
    set(MASON_REPOSITORY "https://mason-binaries.s3.amazonaws.com")
endif()

mason_detect_platform()

# Execute commands if CMake is run in command mode
if (CMAKE_ARGC)
    # Collect remaining arguments for passing to mason_use
    set(_MASON_ARGS)
    if (${CMAKE_ARGC} LESS 5)
        message(FATAL_ERROR "Usage: mason.sh [install|prefix] PACKAGE VERSION")
    endif()

    if (${CMAKE_ARGV3} STREQUAL "install")
        # Install the package
        mason_use(${CMAKE_ARGV4} VERSION ${CMAKE_ARGV5})
    elseif (${CMAKE_ARGV3} STREQUAL "prefix")
        set(PKG_PREFIX "${MASON_PACKAGE_DIR}/${MASON_PLATFORM}-${MASON_PLATFORM_VERSION}/${CMAKE_ARGV4}/${CMAKE_ARGV5}")
        # CMake can't write to stdout with message()
        execute_process(COMMAND ${CMAKE_COMMAND} -E echo "${PKG_PREFIX}")
    else()
        message(FATAL_ERROR "Usage: mason.sh [install|prefix] PACKAGE VERSION")
    endif()

endif()
