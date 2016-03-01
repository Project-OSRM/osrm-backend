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

#include <cstdint>

namespace protozero {

    /**
     * The type used for field tags (field numbers).
     */
    typedef uint32_t pbf_tag_type;

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
     * The type used for length values, such as the length of a field.
     */
    typedef uint32_t pbf_length_type;

} // end namespace protozero

#endif // PROTOZERO_TYPES_HPP
