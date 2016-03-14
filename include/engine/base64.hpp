#ifndef OSRM_BASE64_HPP
#define OSRM_BASE64_HPP

#include <string>
#include <iterator>
#include <type_traits>

#include <cstddef>
#include <climits>

#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/range/algorithm/copy.hpp>

// RFC 4648 "The Base16, Base32, and Base64 Data Encodings"
// See: https://tools.ietf.org/html/rfc4648
// Implementation adapted from: http://stackoverflow.com/a/28471421

// The C++ standard guarantees none of this by default, but we need it in the following.
static_assert(CHAR_BIT == 8u, "we assume a byte holds 8 bits");
static_assert(sizeof(char) == 1u, "we assume a char is one byte large");

namespace osrm
{
namespace engine
{

// Encoding Implementation

// Encodes a chunk of memory to Base64.
inline std::string encodeBase64(const unsigned char *first, std::size_t size)
{
    using namespace boost::archive::iterators;

    const std::string bytes{first, first + size};

    using Iter = base64_from_binary<transform_width<std::string::const_iterator, 6, 8>>;

    Iter view_first{begin(bytes)};
    Iter view_last{end(bytes)};

    std::string encoded{view_first, view_last};

    return encoded.append((3 - size % 3) % 3, '=');
}

// C++11 standard 3.9.1/1: Plain char, signed char, and unsigned char are three distinct types

// Overload for signed char catches (not only but also) C-string literals.
inline std::string encodeBase64(const signed char *first, std::size_t size)
{
    return encodeBase64(reinterpret_cast<const unsigned char *>(first), size);
}

// Overload for char catches (not only but also) C-string literals.
inline std::string encodeBase64(const char *first, std::size_t size)
{
    return encodeBase64(reinterpret_cast<const unsigned char *>(first), size);
}

// Convenience specialization, encoding from string instead of byte-dumping it.
inline std::string encodeBase64(const std::string &x) { return encodeBase64(x.data(), x.size()); }

// Encode any sufficiently trivial object to Base64.
template <typename T> std::string encodeBase64Bytewise(const T &x)
{
#if not defined __GNUC__ or __GNUC__ > 4
    static_assert(std::is_trivially_copyable<T>::value, "requires a trivially copyable type");
#endif

    return encodeBase64(reinterpret_cast<const unsigned char *>(&x), sizeof(T));
}

// Decoding Implementation

// Decodes into a chunk of memory that is at least as large as the input.
template <typename OutputIter> void decodeBase64(const std::string &encoded, OutputIter out)
{
    using namespace boost::archive::iterators;
    using namespace boost::algorithm;

    using Iter = transform_width<binary_from_base64<std::string::const_iterator>, 8, 6>;

    Iter view_first{begin(encoded)};
    Iter view_last{end(encoded)};

    const auto null = [](const unsigned char c)
    {
        return c == '\0';
    };

    const auto bytes = trim_right_copy_if(std::string{view_first, view_last}, null);

    boost::copy(bytes, out);
}

// Convenience specialization, filling string instead of byte-dumping into it.
inline std::string decodeBase64(const std::string &encoded)
{
    std::string rv;

    decodeBase64(encoded, std::back_inserter(rv));

    return rv;
}

// Decodes from Base 64 to any sufficiently trivial object.
template <typename T> T decodeBase64Bytewise(const std::string &encoded)
{
#if not defined __GNUC__ or __GNUC__ > 4
    static_assert(std::is_trivially_copyable<T>::value, "requires a trivially copyable type");
#endif

    T x;

    decodeBase64(encoded, reinterpret_cast<unsigned char *>(&x));

    return x;
}

} // ns engine
} // ns osrm

#endif /* OSRM_BASE64_HPP */
