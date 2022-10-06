#ifndef VTZERO_VERSION_HPP
#define VTZERO_VERSION_HPP

/*****************************************************************************

vtzero - Tiny and fast vector tile decoder and encoder in C++.

This file is from https://github.com/mapbox/vtzero where you can find more
documentation.

*****************************************************************************/

/**
 * @file version.hpp
 *
 * @brief Contains the version number macros for the vtzero library.
 */

/// The major version number
#define VTZERO_VERSION_MAJOR 1

/// The minor version number
#define VTZERO_VERSION_MINOR 0

/// The patch number
#define VTZERO_VERSION_PATCH 1

/// The complete version number
#define VTZERO_VERSION_CODE                                      \
    (VTZERO_VERSION_MAJOR * 10000 + VTZERO_VERSION_MINOR * 100 + \
     VTZERO_VERSION_PATCH)

/// Version number as string
#define VTZERO_VERSION_STRING "1.0.1"

#endif // VTZERO_VERSION_HPP
