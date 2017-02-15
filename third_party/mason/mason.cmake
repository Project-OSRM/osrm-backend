string(RANDOM LENGTH 16 MASON_INVOCATION)

# Directory where Mason packages are located; typically ends with mason_packages
if (NOT MASON_PACKAGE_DIR)
    set(MASON_PACKAGE_DIR "${CMAKE_SOURCE_DIR}/mason_packages")
endif()

# URL prefix of where packages are located.
if (NOT MASON_REPOSITORY)
    set(MASON_REPOSITORY "https://mason-binaries.s3.amazonaws.com")
endif()

# Path to Mason executable
if (NOT MASON_COMMAND)
    set(MASON_COMMAND "${CMAKE_SOURCE_DIR}/.mason/mason")
endif()

# Determine platform
# we call uname -s manually here since
# CMAKE_HOST_SYSTEM_NAME will not be defined before the project() call
execute_process(
    COMMAND uname -s
    OUTPUT_VARIABLE UNAME_S
    OUTPUT_STRIP_TRAILING_WHITESPACE)

if(NOT MASON_PLATFORM)
    if (UNAME_S STREQUAL "Darwin")
        set(MASON_PLATFORM "macos")
    else()
        set(MASON_PLATFORM "linux")
    endif()
endif()


# Determine platform version string
if(MASON_PLATFORM STREQUAL "ios")
    set(MASON_PLATFORM_VERSION "8.0") # Deployment target version
elseif(MASON_PLATFORM STREQUAL "android")
    if (ANDROID_ABI STREQUAL "armeabi")
        set(MASON_PLATFORM_VERSION "arm-v5-9")
    elseif(ANDROID_ABI STREQUAL "arm64-v8a")
        set(MASON_PLATFORM_VERSION "arm-v8-21")
    elseif(ANDROID_ABI STREQUAL "x86")
        set(MASON_PLATFORM_VERSION "x86-9")
    elseif(ANDROID_ABI STREQUAL "x86_64")
        set(MASON_PLATFORM_VERSION "x86-64-21")
    elseif(ANDROID_ABI STREQUAL "mips")
        set(MASON_PLATFORM_VERSION "mips-9")
    elseif(ANDROID_ABI STREQUAL "mips64")
        set(MASON_PLATFORM_VERSION "mips64-21")
    else()
        set(MASON_PLATFORM_VERSION "arm-v7-9")
    endif()
elseif(NOT MASON_PLATFORM_VERSION)
    execute_process(
        COMMAND uname -m
        OUTPUT_VARIABLE MASON_PLATFORM_VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE)
endif()

if(MASON_PLATFORM STREQUAL "macos")
    set(MASON_PLATFORM "osx")
endif()

set(ENV{MASON_PLATFORM} "${MASON_PLATFORM}")
set(ENV{MASON_PLATFORM_VERSION} "${MASON_PLATFORM_VERSION}")

include(CMakeParseArguments)

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
                message(STATUS "[Mason] Downloading package ${_URL}...")

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
            message(STATUS "[Mason] Unpacking package to ${_INSTALL_PATH_RELATIVE}...")
            file(MAKE_DIRECTORY "${_INSTALL_PATH}")
            execute_process(
                COMMAND ${CMAKE_COMMAND} -E tar xzf "${_CACHE_PATH}"
                WORKING_DIRECTORY "${_INSTALL_PATH}")
        endif()

        # Create a config file if it doesn't exist in the package
        # TODO: remove this once all packages have a mason.ini file
        if(NOT EXISTS "${_INSTALL_PATH}/mason.ini")
            # Change pkg-config files
            file(GLOB_RECURSE _PKGCONFIG_FILES "${_INSTALL_PATH}/*.pc")
            foreach(_PKGCONFIG_FILE IN ITEMS ${_PKGCONFIG_FILES})
                file(READ "${_PKGCONFIG_FILE}" _PKGCONFIG_FILE_CONTENT)
                string(REGEX REPLACE "(^|\n)prefix=[^\n]*" "\\1prefix=${_INSTALL_PATH}" _PKGCONFIG_FILE_CONTENT "${_PKGCONFIG_FILE_CONTENT}")
                file(WRITE "${_PKGCONFIG_FILE}" "${_PKGCONFIG_FILE_CONTENT}")
            endforeach()

            if(NOT EXISTS "${MASON_COMMAND}")
                message(FATAL_ERROR "[Mason] Could not find Mason command at ${MASON_COMMAND}")
            endif()

            set(_FAILED)
            set(_ERROR)
            execute_process(
                COMMAND ${MASON_COMMAND} config ${_PACKAGE} ${_VERSION}
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                OUTPUT_FILE "${_INSTALL_PATH}/mason.ini"
                RESULT_VARIABLE _FAILED
                ERROR_VARIABLE _ERROR)
            if(_FAILED)
                message(FATAL_ERROR "[Mason] Could not get configuration for package ${_PACKAGE} ${_VERSION}: ${_ERROR}")
            endif()
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
