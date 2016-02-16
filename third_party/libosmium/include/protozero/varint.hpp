#ifndef PROTOZERO_VARINT_HPP
#define PROTOZERO_VARINT_HPP

/*****************************************************************************

protozero - Minimalistic protocol buffer decoder and encoder in C++.

This file is from https://github.com/mapbox/protozero where you can find more
documentation.

*****************************************************************************/

/**
 * @file varint.hpp
 *
 * @brief Contains low-level varint and zigzag encoding and decoding functions.
 */

#include <cstdint>

#include <protozero/exception.hpp>

namespace protozero {

/**
 * The maximum length of a 64bit varint.
 */
const int8_t max_varint_length = sizeof(uint64_t) * 8 / 7 + 1;

// from https://github.com/facebook/folly/blob/master/folly/Varint.h
/**
 * Decode a 64bit varint.
 *
 * Strong exception guarantee: if there is an exception the data pointer will
 * not be changed.
 *
 * @param[in,out] data Pointer to pointer to the input data. After the function
 *        returns this will point to the next data to be read.
 * @param[in] end Pointer one past the end of the input data.
 * @returns The decoded integer
 * @throws varint_too_long_exception if the varint is longer then the maximum
 *         length that would fit in a 64bit int. Usually this means your data
 *         is corrupted or you are trying to read something as a varint that
 *         isn't.
 * @throws end_of_buffer_exception if the *end* of the buffer was reached
 *         before the end of the varint.
 */
inline uint64_t decode_varint(const char** data, const char* end) {
    const int8_t* begin = reinterpret_cast<const int8_t*>(*data);
    const int8_t* iend = reinterpret_cast<const int8_t*>(end);
    const int8_t* p = begin;
    uint64_t val = 0;

    if (iend - begin >= max_varint_length) {  // fast path
        do {
            int64_t b;
            b = *p++; val  = uint64_t((b & 0x7f)      ); if (b >= 0) break;
            b = *p++; val |= uint64_t((b & 0x7f) <<  7); if (b >= 0) break;
            b = *p++; val |= uint64_t((b & 0x7f) << 14); if (b >= 0) break;
            b = *p++; val |= uint64_t((b & 0x7f) << 21); if (b >= 0) break;
            b = *p++; val |= uint64_t((b & 0x7f) << 28); if (b >= 0) break;
            b = *p++; val |= uint64_t((b & 0x7f) << 35); if (b >= 0) break;
            b = *p++; val |= uint64_t((b & 0x7f) << 42); if (b >= 0) break;
            b = *p++; val |= uint64_t((b & 0x7f) << 49); if (b >= 0) break;
            b = *p++; val |= uint64_t((b & 0x7f) << 56); if (b >= 0) break;
            b = *p++; val |= uint64_t((b & 0x7f) << 63); if (b >= 0) break;
            throw varint_too_long_exception();
        } while (false);
    } else {
        int shift = 0;
        while (p != iend && *p < 0) {
            val |= uint64_t(*p++ & 0x7f) << shift;
            shift += 7;
        }
        if (p == iend) {
            throw end_of_buffer_exception();
        }
        val |= uint64_t(*p++) << shift;
    }

    *data = reinterpret_cast<const char*>(p);
    return val;
}

/**
 * Varint-encode a 64bit integer.
 */
template <typename OutputIterator>
inline int write_varint(OutputIterator data, uint64_t value) {
    int n=1;

    while (value >= 0x80) {
        *data++ = char((value & 0x7f) | 0x80);
        value >>= 7;
        ++n;
    }
    *data++ = char(value);

    return n;
}

/**
 * ZigZag encodes a 32 bit integer.
 */
inline uint32_t encode_zigzag32(int32_t value) noexcept {
    return (static_cast<uint32_t>(value) << 1) ^ (static_cast<uint32_t>(value >> 31));
}

/**
 * ZigZag encodes a 64 bit integer.
 */
inline uint64_t encode_zigzag64(int64_t value) noexcept {
    return (static_cast<uint64_t>(value) << 1) ^ (static_cast<uint64_t>(value >> 63));
}

/**
 * Decodes a 32 bit ZigZag-encoded integer.
 */
inline int32_t decode_zigzag32(uint32_t value) noexcept {
    return int32_t(value >> 1) ^ -int32_t(value & 1);
}

/**
 * Decodes a 64 bit ZigZag-encoded integer.
 */
inline int64_t decode_zigzag64(uint64_t value) noexcept {
    return int64_t(value >> 1) ^ -int64_t(value & 1);
}

} // end namespace protozero

#endif // PROTOZERO_VARINT_HPP
