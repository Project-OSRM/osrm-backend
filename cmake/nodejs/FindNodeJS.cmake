# Copyright (c) 2015, Colin Taylor
# Permission to use, copy, modify, and/or distribute this software for any 
# purpose with or without fee is hereby granted, provided that the above 
# copyright notice and this permission notice appear in all copies.
# 
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH 
# REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY 
# AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, 
# INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM 
# LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR 
# OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR 
# PERFORMANCE OF THIS SOFTWARE.

# FindNodeJS.cmake CMake module vendored from the node-cmake project (v1.2).

# This script uses CMake 3.1+ features
if(CMAKE_MINIMUM_REQUIRED_VERSION VERSION_LESS 3.1.0)
    message(FATAL_ERROR "FindNodeJS.cmake uses CMake 3.1+ features")
endif()

# Force a build type to be set (ignored on config based generators)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

# Capture module information
set(NodeJS_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR})
get_filename_component(NodeJS_MODULE_NAME ${NodeJS_MODULE_PATH} NAME)

# Allow users to specify the installed location of the Node.js package
set(NodeJS_ROOT_DIR "" CACHE PATH 
    "The root directory of the node.js installed package")

# Allow users to specify that downloaded sources should be used
option(NodeJS_DOWNLOAD "Download the required source files" Off)

# Allow users to force downloading of node packages
option(NodeJS_FORCE_DOWNLOAD "Download the source files every time" Off)

# Allow users to force archive extraction
option(NodeJS_FORCE_EXTRACT "Extract the archive every time" Off)

# Make libc++ the default when compiling with clang
option(NodeJS_USE_CLANG_STDLIB "Use libc++ when compiling with clang" On)
if(APPLE)
    set(NodeJS_USE_CLANG_STDLIB On CACHE BOOL "" FORCE)
endif()

if(WIN32)
    # Allow users to specify that the executable should be downloaded
    option(NodeJS_DOWNLOAD_EXECUTABLE
        "Download matching executable if available" Off
    )
endif()

# Try to find the node.js executable
# The node executable under linux may not be the correct program
find_program(NodeJS_EXECUTABLE 
    NAMES node
    PATHS ${NodeJS_ROOT_DIR}
    PATH_SUFFIXES nodejs node
)
set(NodeJS_VALIDATE_EXECUTABLE 1)
if(NodeJS_EXECUTABLE)
    execute_process(
        COMMAND ${NodeJS_EXECUTABLE} --version
        RESULT_VARIABLE NodeJS_VALIDATE_EXECUTABLE
        OUTPUT_VARIABLE NodeJS_INSTALLED_VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    execute_process(
        COMMAND ${NodeJS_EXECUTABLE} -p "process.platform"
        OUTPUT_VARIABLE NodeJS_INSTALLED_PLATFORM
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    execute_process(
        COMMAND ${NodeJS_EXECUTABLE} -p "process.arch"
        OUTPUT_VARIABLE NodeJS_INSTALLED_ARCH
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
endif()

# If node isn't the node.js binary, try the nodejs binary
if(NOT NodeJS_VALIDATE_EXECUTABLE EQUAL 0)
    find_program(NodeJS_EXECUTABLE
        NAMES nodejs
        PATHS ${NodeJS_ROOT_DIR}
        PATH_SUFFIXES nodejs node
    )
    set(NodeJS_VALIDATE_EXECUTABLE 1)
    if(NodeJS_EXECUTABLE)
        execute_process(
            COMMAND ${NodeJS_EXECUTABLE} --version
            RESULT_VARIABLE NodeJS_VALIDATE_EXECUTABLE
            OUTPUT_VARIABLE NodeJS_INSTALLED_VERSION
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
    endif()

    if(NOT NodeJS_VALIDATE_EXECUTABLE EQUAL 0)
        message(WARNING "Node.js executable could not be found. \
        Set NodeJS_ROOT_DIR to the installed location of the executable or \
        install Node.js to its default location.")
    endif()
endif()

# Determine if a variant is set in the components
list(APPEND NodeJS_OTHER_COMPONENTS
    X64 IA32 ARM WIN32 LINUX DARWIN
)
set(NodeJS_COMPONENTS_CONTAINS_VARIANT False)
foreach(NodeJS_COMPONENT ${NodeJS_FIND_COMPONENTS})
    list(FIND NodeJS_OTHER_COMPONENTS ${NodeJS_COMPONENT} NodeJS_OTHER_INDEX)
    if(NodeJS_OTHER_INDEX EQUAL -1)
        set(NodeJS_COMPONENTS_CONTAINS_VARIANT True)
        break()
    endif()
endforeach()

# Get the targeted version of Node.js (or one of its derivatives)
if(NOT NodeJS_VERSION)
    if(NodeJS_FIND_VERSION)
        set(NodeJS_VERSION ${NodeJS_FIND_VERSION})
    elseif(NodeJS_INSTALLED_VERSION AND NOT NodeJS_COMPONENTS_CONTAINS_VARIANT)
        string(SUBSTRING ${NodeJS_INSTALLED_VERSION} 1 -1 NodeJS_VERSION)
    else()
        message(FATAL_ERROR "Node.js version is not set. Set the VERSION \
        property of the find_package command to the required version of the \
        Node.js sources")
    endif()
endif()

# Determine the target platform for the compiled module
# Uses several mechanisms in order:
# 
# 1. CMake cache (allows overriding on the command line)
# 2. Node architecture when binary is available
# 3. CMake architecture
# 
set(NodeJS_PLATFORM "" CACHE STRING "Target node.js platform for module")
if(NOT NodeJS_PLATFORM)
    if(NodeJS_EXECUTABLE)
        set(NodeJS_PLATFORM ${NodeJS_INSTALLED_PLATFORM})
    elseif(WIN32)
        set(NodeJS_PLATFORM "win32")
    elseif(UNIX)
        if(APPLE)
            set(NodeJS_PLATFORM "darwin")
        else()
            set(NodeJS_PLATFORM "linux")
        endif()
    else()
        message(FATAL_ERROR "Node.js platform is not set. Add the platform \
        to the find_package components section or set NodeJS_PLATFORM in the \
        cache.")
    endif()
endif()

# Convenience variables for the platform type
if(NodeJS_PLATFORM STREQUAL "win32")
    set(NodeJS_PLATFORM_WIN32 True)
    set(NodeJS_PLATFORM_LINUX False)
    set(NodeJS_PLATFORM_DARWIN False)
elseif(NodeJS_PLATFORM STREQUAL "linux")
    set(NodeJS_PLATFORM_WIN32 False)
    set(NodeJS_PLATFORM_LINUX True)
    set(NodeJS_PLATFORM_DARWIN False)
elseif(NodeJS_PLATFORM STREQUAL "darwin")
    set(NodeJS_PLATFORM_WIN32 False)
    set(NodeJS_PLATFORM_LINUX False)
    set(NodeJS_PLATFORM_DARWIN True)
endif()

# Determine the target architecture for the compiled module
# Uses several mechanisms in order:
# 
# 1. CMake cache (allows overriding on the command line)
# 2. Node architecture when binary is available
# 3. Compiler architecture under MSVC
# 
set(NodeJS_ARCH "" CACHE STRING "Target node.js architecture for module")
if(NOT NodeJS_ARCH)
    if(NodeJS_EXECUTABLE)
        set(NodeJS_ARCH ${NodeJS_INSTALLED_ARCH})
    elseif(MSVC)
        if(CMAKE_CL_64)
            set(NodeJS_ARCH "x64")
        else()
            set(NodeJS_ARCH "ia32")
        endif()
    else()
        message(FATAL_ERROR "Node.js architecture is not set. Add the \
        architecture to the find_package components section or set NodeJS_ARCH \
        in the cache.")
    endif()
endif()

# Convenience variables for the architecture
if(NodeJS_ARCH STREQUAL "x64")
    set(NodeJS_ARCH_X64 True)
    set(NodeJS_ARCH_IA32 False)
    set(NodeJS_ARCH_ARM False)
elseif(NodeJS_ARCH STREQUAL "ia32")
    set(NodeJS_ARCH_X64 False)
    set(NodeJS_ARCH_IA32 True)
    set(NodeJS_ARCH_ARM False)
elseif(NodeJS_ARCH STREQUAL "arm")
    set(NodeJS_ARCH_X64 False)
    set(NodeJS_ARCH_IA32 False)
    set(NodeJS_ARCH_ARM True)
endif()

# Include helper functions
include(util/NodeJSUtil)

# Default variant name
# Used by the installed header comparison below
set(NodeJS_DEFAULT_VARIANT_NAME "node.js")

# Variables for Node.js artifacts across variants
# Specify all of these variables for each new variant
set(NodeJS_VARIANT_NAME "")       # The printable name of the variant
set(NodeJS_VARIANT_BASE "")       # A file name safe version of the variant
set(NodeJS_URL "")                # The URL for the artifacts
set(NodeJS_SOURCE_PATH "")        # The URL path of the source archive
set(NodeJS_CHECKSUM_PATH "")      # The URL path of the checksum file
set(NodeJS_CHECKSUM_TYPE "")      # The checksum type (algorithm)
set(NodeJS_WIN32_LIBRARY_PATH "") # The URL path of the windows library
set(NodeJS_WIN32_BINARY_PATH "")  # The URL path of the windows executable
set(NodeJS_WIN32_LIBRARY_NAME "") # The name of the windows library
set(NodeJS_WIN32_BINARY_NAME "")  # The name of the windows executable

set(NodeJS_DEFAULT_INCLUDE True)  # Enable default include behavior
set(NodeJS_DEFAULT_LIBS True)     # Include the default libraries
set(NodeJS_HAS_WIN32_PREFIX True) # Does the variant use library prefixes
set(NodeJS_HAS_WIN32_BINARY True) # Does the variant have win32 executables
set(NodeJS_HAS_OPENSSL True)      # Does the variant include openssl headers
set(NodeJS_HEADER_VERSION 0.12.7) # Version after header-only archives start
set(NodeJS_SHA256_VERSION 0.7.0)  # Version after sha256 checksums start
set(NodeJS_PREFIX_VERSION 0.12.7) # Version after windows prefixing starts
set(NodeJS_CXX11R_VERSION 0.12.7) # Version after c++11 is required
set(NodeJS_SOURCE_INCLUDE True)   # Use the include paths from a source archive
set(NodeJS_HEADER_INCLUDE False)  # Use the include paths from a header archive
set(NodeJS_INCLUDE_PATHS "")      # Set of header dirs inside the source archive
set(NodeJS_LIBRARIES "")          # The set of libraries to link with addon
set(NodeJS_WIN32_DELAYLOAD "")    # Set of executables to delayload on windows

# NodeJS variants
# Selects download target based on configured component
# Include NodeJS last to provide default configurations when omitted
file(
    GLOB NodeJS_SUPPORTED_VARIANTS
    RELATIVE ${CMAKE_CURRENT_LIST_DIR}/variants
    ${CMAKE_CURRENT_LIST_DIR}/variants/*
)
foreach(NodeJS_SUPPORTED_VARIANT ${NodeJS_SUPPORTED_VARIANTS})
    get_filename_component(NodeJS_SUPPORTED_VARIANT_NAME
        ${NodeJS_SUPPORTED_VARIANT} NAME_WE
    )
    if(NOT NodeJS_SUPPORTED_VARIANT_NAME STREQUAL "NodeJS")
        include(variants/${NodeJS_SUPPORTED_VARIANT_NAME})
    endif()
endforeach()
include(variants/NodeJS)

# Populate version variables, including version components
set(NodeJS_VERSION_STRING "v${NodeJS_VERSION}")

# Populate the remaining version variables
string(REPLACE "." ";" NodeJS_VERSION_PARTS ${NodeJS_VERSION})
list(GET NodeJS_VERSION_PARTS 0 NodeJS_VERSION_MAJOR)
list(GET NodeJS_VERSION_PARTS 1 NodeJS_VERSION_MINOR)
list(GET NodeJS_VERSION_PARTS 2 NodeJS_VERSION_PATCH)

# If the version we're looking for is the version that is installed,
# try finding the required headers. Don't do this under windows (where
# headers are not part of the installed content), when the user has
# specified that headers should be downloaded or when using a variant other
# than the default
if((NOT NodeJS_PLATFORM_WIN32) AND (NOT NodeJS_DOWNLOAD) AND
    NodeJS_VARIANT_NAME STREQUAL NodeJS_DEFAULT_VARIANT_NAME AND
    NodeJS_INSTALLED_VERSION STREQUAL NodeJS_VERSION_STRING AND
    NodeJS_INSTALLED_PLATFORM STREQUAL NodeJS_PLATFORM AND
    NodeJS_INSTALLED_ARCH STREQUAL NodeJS_ARCH)
    # node.h is really generic and too easy for cmake to find the wrong
    # file, so use the directory as a guard, and then just tack it on to
    # the actual path
    # 
    # Specifically ran into this under OSX, where python contains a node.h
    # that gets found instead
    find_path(NodeJS_INCLUDE_PARENT node/node.h)
    set(NodeJS_INCLUDE_DIRS ${NodeJS_INCLUDE_PARENT}/node)

    # Under all systems that support this, there are no libraries required
    # for linking (symbols are resolved via the main executable at runtime)
    set(NodeJS_LIBRARIES "")

# Otherwise, headers and required libraries must be downloaded to the project
# to supplement what is installed
else()
    # Create a folder for downloaded artifacts
    set(NodeJS_DOWNLOAD_PATH 
        ${CMAKE_CURRENT_BINARY_DIR}/${NodeJS_VARIANT_BASE}
    )
    set(NodeJS_DOWNLOAD_PATH ${NodeJS_DOWNLOAD_PATH}-${NodeJS_VERSION_STRING})
    file(MAKE_DIRECTORY ${NodeJS_DOWNLOAD_PATH})

    # Download the checksum file for validating all other downloads
    # Conveniently, if this doesn't download correctly, the setup fails
    # due to checksum failures
    set(NodeJS_CHECKSUM_FILE ${NodeJS_DOWNLOAD_PATH}/CHECKSUM)
    nodejs_download(
        ${NodeJS_URL}/${NodeJS_CHECKSUM_PATH}
        ${NodeJS_CHECKSUM_FILE}
        ${NodeJS_FORCE_DOWNLOAD}
    )
    file(READ ${NodeJS_CHECKSUM_FILE} NodeJS_CHECKSUM_DATA)

    # Download and extract the main source archive
    set(NodeJS_SOURCE_FILE ${NodeJS_DOWNLOAD_PATH}/headers.tar.gz)
    nodejs_checksum(
        ${NodeJS_CHECKSUM_DATA} ${NodeJS_SOURCE_PATH} NodeJS_SOURCE_CHECKSUM
    )
    nodejs_download(
        ${NodeJS_URL}/${NodeJS_SOURCE_PATH}
        ${NodeJS_SOURCE_FILE}
        ${NodeJS_SOURCE_CHECKSUM}
        ${NodeJS_CHECKSUM_TYPE}
        ${NodeJS_FORCE_DOWNLOAD}
    )
    set(NodeJS_HEADER_PATH ${NodeJS_DOWNLOAD_PATH}/src)
    nodejs_extract(
        ${NodeJS_SOURCE_FILE}
        ${NodeJS_HEADER_PATH}
        ${NodeJS_FORCE_EXTRACT}
    )

    # Populate include directories from the extracted source archive
    foreach(NodeJS_HEADER_BASE ${NodeJS_INCLUDE_PATHS})
        set(NodeJS_INCLUDE_DIR ${NodeJS_HEADER_PATH}/${NodeJS_HEADER_BASE})
        if(NOT EXISTS ${NodeJS_INCLUDE_DIR})
            message(FATAL_ERROR "Include does not exist: ${NodeJS_INCLUDE_DIR}")
        endif()
        list(APPEND NodeJS_INCLUDE_DIRS ${NodeJS_INCLUDE_DIR})
    endforeach()

    # Download required library files when targeting windows
    if(NodeJS_PLATFORM_WIN32)
        # Download the windows library
        set(NodeJS_WIN32_LIBRARY_FILE 
            ${NodeJS_DOWNLOAD_PATH}/lib/${NodeJS_ARCH}
        )
        set(NodeJS_WIN32_LIBRARY_FILE 
            ${NodeJS_WIN32_LIBRARY_FILE}/${NodeJS_WIN32_LIBRARY_NAME}
        )
        nodejs_checksum(
            ${NodeJS_CHECKSUM_DATA} ${NodeJS_WIN32_LIBRARY_PATH} 
            NodeJS_WIN32_LIBRARY_CHECKSUM
        )
        nodejs_download(
            ${NodeJS_URL}/${NodeJS_WIN32_LIBRARY_PATH}
            ${NodeJS_WIN32_LIBRARY_FILE}
            ${NodeJS_WIN32_LIBRARY_CHECKSUM}
            ${NodeJS_CHECKSUM_TYPE}
            ${NodeJS_FORCE_DOWNLOAD}
        )
        list(APPEND NodeJS_LIBRARIES ${NodeJS_WIN32_LIBRARY_FILE})

        # If provided, download the windows executable
        if(NodeJS_WIN32_BINARY_PATH AND 
            NodeJS_DOWNLOAD_EXECUTABLE)
            set(NodeJS_WIN32_BINARY_FILE 
                ${NodeJS_DOWNLOAD_PATH}/lib/${NodeJS_ARCH}
            )
            set(NodeJS_WIN32_BINARY_FILE 
                ${NodeJS_WIN32_BINARY_FILE}/${NodeJS_WIN32_BINARY_NAME}
            )
            nodejs_checksum(
                ${NodeJS_CHECKSUM_DATA} ${NodeJS_WIN32_BINARY_PATH} 
                NodeJS_WIN32_BINARY_CHECKSUM
            )
            nodejs_download(
                ${NodeJS_URL}/${NodeJS_WIN32_BINARY_PATH}
                ${NodeJS_WIN32_BINARY_FILE}
                ${NodeJS_WIN32_BINARY_CHECKSUM}
                ${NodeJS_CHECKSUM_TYPE}
                ${NodeJS_FORCE_DOWNLOAD}
            )
        endif()
    endif()
endif()

# Support windows delay loading
if(NodeJS_PLATFORM_WIN32)
    list(APPEND NodeJS_LINK_FLAGS /IGNORE:4199)
    set(NodeJS_WIN32_DELAYLOAD_CONDITION "")
    foreach(NodeJS_WIN32_DELAYLOAD_BINARY ${NodeJS_WIN32_DELAYLOAD})
        list(APPEND NodeJS_LINK_FLAGS
            /DELAYLOAD:${NodeJS_WIN32_DELAYLOAD_BINARY}
        )
        list(APPEND NodeJS_WIN32_DELAYLOAD_CONDITION
            "_stricmp(info->szDll, \"${NodeJS_WIN32_DELAYLOAD_BINARY}\") != 0"
        )
    endforeach()
    string(REPLACE ";" " &&\n     " 
        NodeJS_WIN32_DELAYLOAD_CONDITION
        "${NodeJS_WIN32_DELAYLOAD_CONDITION}"
    )
    configure_file(
        ${NodeJS_MODULE_PATH}/src/win_delay_load_hook.c
        ${CMAKE_CURRENT_BINARY_DIR}/win_delay_load_hook.c @ONLY
    )
    list(APPEND NodeJS_ADDITIONAL_SOURCES
        ${CMAKE_CURRENT_BINARY_DIR}/win_delay_load_hook.c
    )
endif()

# Allow undefined symbols on OSX
if(NodeJS_PLATFORM_DARWIN)
    list(APPEND NodeJS_LINK_FLAGS "-undefined dynamic_lookup")
endif()

# Use libc++ when clang is the compiler by default
if(NodeJS_USE_CLANG_STDLIB AND CMAKE_CXX_COMPILER_ID MATCHES ".*Clang.*")
    list(APPEND NodeJS_COMPILE_OPTIONS -stdlib=libc++)
endif()

# Require c++11 support after a specific point, but only if the user hasn't
# specified an override
if(NOT NodeJS_CXX_STANDARD) 
    if(NodeJS_VERSION VERSION_GREATER NodeJS_CXX11R_VERSION)
        set(NodeJS_CXX_STANDARD 11)
    else()
        set(NodeJS_CXX_STANDARD 98)
    endif()
endif()

# Set required definitions
list(APPEND NodeJS_DEFINITIONS BUILDING_NODE_EXTENSION)
if(NodeJS_PLATFORM_DARWIN)
    list(APPEND NodeJS_DEFINITIONS _DARWIN_USE_64_BIT_INODE=1)
endif()
if(NOT NodeJS_PLATFORM_WIN32)
    list(APPEND NodeJS_DEFINITIONS
        _LARGEFILE_SOURCE
        _FILE_OFFSET_BITS=64
    )
endif()

function(add_nodejs_module NAME)
    # Build a shared library for the module
    add_library(${NAME} SHARED ${ARGN} ${NodeJS_ADDITIONAL_SOURCES})

    # Include required headers
    # Find and include Nan as well (always available as its a 
    # dependency of this module)
    nodejs_find_module_fallback(nan ${CMAKE_CURRENT_SOURCE_DIR} NAN_PATH)
    target_include_directories(${NAME} 
        PUBLIC ${NodeJS_INCLUDE_DIRS}
        PUBLIC ${NAN_PATH}
    )

    # Set module properties
    # This ensures proper naming of the module library across all platforms
    get_target_property(COMPILE_OPTIONS ${NAME} COMPILE_OPTIONS)
    if(NOT COMPILE_OPTIONS)
        set(COMPILE_OPTIONS "")
    endif()
    set(COMPILE_OPTIONS ${COMPILE_OPTIONS} ${NodeJS_COMPILE_OPTIONS})
    get_target_property(LINK_FLAGS ${NAME} LINK_FLAGS)
    if(NOT LINK_FLAGS)
        set(LINK_FLAGS "")
    endif()
    foreach(NodeJS_LINK_FLAG ${NodeJS_LINK_FLAGS})
        set(LINK_FLAGS "${LINK_FLAGS} ${NodeJS_LINK_FLAG}")
    endforeach()
    set_target_properties(${NAME} PROPERTIES
        PREFIX ""
        SUFFIX ".node"
        MACOSX_RPATH ON
        POSITION_INDEPENDENT_CODE TRUE
        COMPILE_OPTIONS "${COMPILE_OPTIONS}"
        LINK_FLAGS "${LINK_FLAGS}"
        CXX_STANDARD_REQUIRED TRUE
        CXX_STANDARD ${NodeJS_CXX_STANDARD}
    )

    # Output the module in a per build type directory
    # This makes builds consistent with visual studio and other generators
    # that build by configuration
    if(NOT CMAKE_CONFIGURATION_TYPES)
        set_property(TARGET ${NAME} PROPERTY LIBRARY_OUTPUT_DIRECTORY
            ${CMAKE_BUILD_TYPE}
        )
    endif()

    # Set any required complier flags
    # Mostly used under windows
    target_compile_definitions(${NAME} PRIVATE ${NodeJS_DEFINITIONS})

    # Link against required NodeJS libraries
    target_link_libraries(${NAME} ${NodeJS_LIBRARIES})
endfunction()

# Write out the configuration for node scripts
configure_file(
    ${NodeJS_MODULE_PATH}/build.json.in
    ${CMAKE_CURRENT_BINARY_DIR}/build.json @ONLY
)

# Make sure we haven't violated the version-to-standard mapping
if(NodeJS_VERSION VERSION_GREATER NodeJS_CXX11R_VERSION AND
    NodeJS_CXX_STANDARD EQUAL 98)
    message(FATAL_ERROR "${NodeJS_VARIANT_NAME} ${NodeJS_VERSION} \
    requires C++11 or newer to build")
endif()

# This is a find_package file, handle the standard invocation
include(FindPackageHandleStandardArgs)
set(NodeJS_TARGET "${NodeJS_VARIANT_NAME} ${NodeJS_PLATFORM}/${NodeJS_ARCH}")
find_package_handle_standard_args(NodeJS
    FOUND_VAR NodeJS_FOUND
    REQUIRED_VARS NodeJS_TARGET NodeJS_INCLUDE_DIRS
    VERSION_VAR NodeJS_VERSION
)

# Mark variables that users shouldn't modify
mark_as_advanced(
    NodeJS_VALIDATE_EXECUTABLE
    NodeJS_OTHER_COMPONENTS
    NodeJS_COMPONENTS_CONTAINS_VARIANT
    NodeJS_COMPONENT
    NodeJS_OTHER_INDEX
    NodeJS_VERSION_STRING
    NodeJS_VERSION_MAJOR
    NodeJS_VERSION_MINOR
    NodeJS_VERSION_PATCH
    NodeJS_VERSION_TWEAK
    NodeJS_PLATFORM
    NodeJS_PLATFORM_WIN32
    NodeJS_PLATFORM_LINUX
    NodeJS_PLATFORM_DARWIN
    NodeJS_ARCH
    NodeJS_ARCH_X64
    NodeJS_ARCH_IA32
    NodeJS_ARCH_ARM
    NodeJS_DEFAULT_VARIANT_NAME
    NodeJS_VARIANT_BASE
    NodeJS_VARIANT_NAME
    NodeJS_URL
    NodeJS_SOURCE_PATH
    NodeJS_CHECKSUM_PATH
    NodeJS_CHECKSUM_TYPE
    NodeJS_WIN32_LIBRARY_PATH
    NodeJS_WIN32_BINARY_PATH
    NodeJS_WIN32_LIBRARY_NAME
    NodeJS_WIN32_BINARY_NAME
    NodeJS_DEFAULT_INCLUDE
    NodeJS_DEFAULT_LIBS
    NodeJS_HAS_WIN32_BINARY
    NodeJS_HEADER_VERSION
    NodeJS_SHA256_VERISON
    NodeJS_PREFIX_VERSION
    NodeJS_SOURCE_INCLUDE
    NodeJS_HEADER_INCLUDE
    NodeJS_INCLUDE_PATHS
    NodeJS_WIN32_DELAYLOAD
    NodeJS_DOWNLOAD_PATH
    NodeJS_CHECKSUM_FILE
    NodeJS_CHECKSUM_DATA
    NodeJS_SOURCE_FILE
    NodeJS_SOURCE_CHECKSUM
    NodeJS_HEADER_PATH
    NodeJS_HEADER_BASE
    NodeJS_INCLUDE_DIR
    NodeJS_WIN32_LIBRARY_FILE
    NodeJS_WIN32_LIBRARY_CHECKSUM
    NodeJS_WIN32_BINARY_FILE
    NodeJS_WIN32_BINARY_CHECKSUM
    NodeJS_NAN_PATH
    NodeJS_LINK_FLAGS
    NodeJS_COMPILE_OPTIONS
    NodeJS_ADDITIONAL_SOURCES
    NodeJS_WIN32_DELAYLOAD_CONDITION
    NodeJS_WIN32_DELAYLOAD_BINARY
    NodeJS_TARGET
)
