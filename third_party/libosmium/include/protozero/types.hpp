#ifndef PROTOZERO_TYPES_HPP
#define PROTOZERO_TYPES_HPP

/*****************************************************************************

protozero - Minimalistic protocol buffer decoder and encoder in C++.

This file is from https://github.com/mapbox/protozero where you can find more
documentation.

*****************************************************************************/

/**
 * @file types.hpp
 *
 * @brief Contains the declaration of low-level types used in the pbf format.
 */

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <utility>

#include <protozero/config.hpp>

namespace protozero {

/**
 * The type used for field tags (field numbers).
 */
using pbf_tag_type = uint32_t;

/**
 * The type used to encode type information.
 * See the table on
 *    https://developers.google.com/protocol-buffers/docs/encoding
 */
enum class pbf_wire_type : uint32_t {
    varint           = 0, // int32/64, uint32/64, sint32/64, bool, enum
    fixed64          = 1, // fixed64, sfixed64, double
    length_delimited = 2, // string, bytes, embedded messages,
                            // packed repeated fields
    fixed32          = 5, // fixed32, sfixed32, float
    unknown          = 99 // used for default setting in this library
};

/**
 * Get the tag and wire type of the current field in one integer suitable
 * for comparison with a switch statement.
 *
 * See pbf_reader.tag_and_type() for an example how to use this.
 */
template <typename T>
constexpr inline uint32_t tag_and_type(T tag, pbf_wire_type wire_type) noexcept {
    return (static_cast<uint32_t>(static_cast<pbf_tag_type>(tag)) << 3) | static_cast<uint32_t>(wire_type);
}

/**
 * The type used for length values, such as the length of a field.
 */
using pbf_length_type = uint32_t;

#ifdef PROTOZERO_USE_VIEW
using data_view = PROTOZERO_USE_VIEW;
#else

/**
 * Holds a pointer to some data and a length.
 *
 * This class is supposed to be compatible with the std::string_view
 * that will be available in C++17.
 */
class data_view {

    const char* m_data;
    std::size_t m_size;

public:

    /**
     * Default constructor. Construct an empty data_view.
     */
    constexpr data_view() noexcept
        : m_data(nullptr),
          m_size(0) {
    }

    /**
     * Create data_view from pointer and size.
     *
     * @param ptr Pointer to the data.
     * @param length Length of the data.
     */
    constexpr data_view(const char* ptr, std::size_t length) noexcept
        : m_data(ptr),
          m_size(length) {
    }

    /**
     * Create data_view from string.
     *
     * @param str String with the data.
     */
    data_view(const std::string& str) noexcept
        : m_data(str.data()),
          m_size(str.size()) {
    }

    /**
     * Create data_view from zero-terminated string.
     *
     * @param ptr Pointer to the data.
     */
    data_view(const char* ptr) noexcept
        : m_data(ptr),
          m_size(std::strlen(ptr)) {
    }

    /**
     * Swap the contents of this object with the other.
     *
     * @param other Other object to swap data with.
     */
    void swap(data_view& other) noexcept {
        using std::swap;
        swap(m_data, other.m_data);
        swap(m_size, other.m_size);
    }

    /// Return pointer to data.
    constexpr const char* data() const noexcept {
        return m_data;
    }

    /// Return length of data in bytes.
    constexpr std::size_t size() const noexcept {
        return m_size;
    }

    /// Returns true if size is 0.
    constexpr bool empty() const noexcept {
        return m_size == 0;
    }

    /**
     * Convert data view to string.
     *
     * @pre Must not be default constructed data_view.
     */
    std::string to_string() const {
        protozero_assert(m_data);
        return std::string{m_data, m_size};
    }

    /**
     * Convert data view to string.
     *
     * @pre Must not be default constructed data_view.
     */
    explicit operator std::string() const {
        protozero_assert(m_data);
        return std::string{m_data, m_size};
    }

}; // class data_view

/**
 * Swap two data_view objects.
 *
 * @param lhs First object.
 * @param rhs Second object.
 */
inline void swap(data_view& lhs, data_view& rhs) noexcept {
    lhs.swap(rhs);
}

/**
 * Two data_view instances are equal if they have the same size and the
 * same content.
 *
 * @param lhs First object.
 * @param rhs Second object.
 */
inline bool operator==(const data_view& lhs, const data_view& rhs) noexcept {
    return lhs.size() == rhs.size() && std::equal(lhs.data(), lhs.data() + lhs.size(), rhs.data());
}

/**
 * Two data_view instances are not equal if they have different sizes or the
 * content differs.
 *
 * @param lhs First object.
 * @param rhs Second object.
 */
inline bool operator!=(const data_view& lhs, const data_view& rhs) noexcept {
    return !(lhs == rhs);
}

#endif


} // end namespace protozero

#endif // PROTOZERO_TYPES_HPP
