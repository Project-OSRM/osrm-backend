#ifndef VTZERO_EXCEPTION_HPP
#define VTZERO_EXCEPTION_HPP

/*****************************************************************************

vtzero - Tiny and fast vector tile decoder and encoder in C++.

This file is from https://github.com/mapbox/vtzero where you can find more
documentation.

*****************************************************************************/

/**
 * @file exception.hpp
 *
 * @brief Contains the exceptions used in the vtzero library.
 */

#include <cstdint>
#include <stdexcept>
#include <string>

namespace vtzero {

    /**
     * Base class for all exceptions directly thrown by the vtzero library.
     */
    class exception : public std::runtime_error {

    public:

        /// Constructor
        explicit exception(const char* message) :
            std::runtime_error(message) {
        }

        /// Constructor
        explicit exception(const std::string& message) :
            std::runtime_error(message) {
        }

    }; // class exception

    /**
     * This exception is thrown when vector tile encoding isn't valid according
     * to the vector tile specification.
     */
    class format_exception : public exception {

    public:

        /// Constructor
        explicit format_exception(const char* message) :
            exception(message) {
        }

        /// Constructor
        explicit format_exception(const std::string& message) :
            exception(message) {
        }

    }; // class format_exception

    /**
     * This exception is thrown when a geometry encoding isn't valid according
     * to the vector tile specification.
     */
    class geometry_exception : public format_exception {

    public:

        /// Constructor
        explicit geometry_exception(const char* message) :
            format_exception(message) {
        }

        /// Constructor
        explicit geometry_exception(const std::string& message) :
            format_exception(message) {
        }

    }; // class geometry_exception

    /**
     * This exception is thrown when a property value is accessed using the
     * wrong type.
     */
    class type_exception : public exception {

    public:

        /// Constructor
        explicit type_exception() :
            exception("wrong property value type") {
        }

    }; // class type_exception

    /**
     * This exception is thrown when an unknown version number is found in the
     * layer.
     */
    class version_exception : public exception {

    public:

        /// Constructor
        explicit version_exception(const uint32_t version) :
            exception(std::string{"unknown vector tile version: "} +
                      std::to_string(version)) {
        }

    }; // version_exception

    /**
     * This exception is thrown when an index into the key or value table
     * in a layer is out of range. This can only happen if the tile data is
     * invalid.
     */
    class out_of_range_exception : public exception {

    public:

        /// Constructor
        explicit out_of_range_exception(const uint32_t index) :
            exception(std::string{"index out of range: "} +
                      std::to_string(index)) {
        }

    }; // out_of_range_exception

} // namespace vtzero

#endif // VTZERO_EXCEPTION_HPP
