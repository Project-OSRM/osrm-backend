#ifndef OSRM_BASE64_HPP
#define OSRM_BASE64_HPP

#include <iterator>
#include <string>
#include <type_traits>
#include <vector>

#include <climits>
#include <cstddef>

#include <boost/algorithm/string/trim.hpp>
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <boost/range/algorithm/copy.hpp>

namespace osrm
{

// RFC 4648 "The Base16, Base32, and Base64 Data Encodings"
// See: https://tools.ietf.org/html/rfc4648

namespace detail
{
// The C++ standard guarantees none of this by default, but we need it in the following.
static_assert(CHAR_BIT == 8u, "we assume a byte holds 8 bits");
static_assert(sizeof(char) == 1u, "we assume a char is one byte large");

using Base64FromBinary = boost::archive::iterators::base64_from_binary<
    boost::archive::iterators::transform_width<const char *, // sequence of chars
                                               6,            // get view of 6 bit
                                               8             // from sequence of 8 bit
                                               >>;

using BinaryFromBase64 = boost::archive::iterators::transform_width<
    boost::archive::iterators::binary_from_base64<std::string::const_iterator>,
    8, // get a view of 8 bit
    6  // from a sequence of 6 bit
    >;
} // namespace detail
namespace engine
{

// Encoding Implementation

// Encodes a chunk of memory to Base64.
inline std::string encodeBase64(const unsigned char *first, std::size_t size)
{
    std::vector<unsigned char> bytes{first, first + size};
    BOOST_ASSERT(!bytes.empty());

    std::size_t bytes_to_pad{0};

    while (bytes.size() % 3 != 0)
    {
        bytes_to_pad += 1;
        bytes.push_back(0);
    }

    BOOST_ASSERT(bytes_to_pad == 0 || bytes_to_pad == 1 || bytes_to_pad == 2);
    BOOST_ASSERT_MSG(0 == bytes.size() % 3, "base64 input data size is not a multiple of 3");

    std::string encoded{detail::Base64FromBinary{bytes.data()},
                        detail::Base64FromBinary{bytes.data() + (bytes.size() - bytes_to_pad)}};

    return encoded.append(bytes_to_pad, '=');
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
#if !defined(__GNUC__) || (__GNUC__ > 4)
    static_assert(std::is_trivially_copyable<T>::value, "requires a trivially copyable type");
#endif

    return encodeBase64(reinterpret_cast<const unsigned char *>(&x), sizeof(T));
}

// Decoding Implementation

// Decodes into a chunk of memory that is at least as large as the input.
template <typename OutputIter> void decodeBase64(const std::string &encoded, OutputIter out)
{
    auto unpadded = encoded;

    const auto num_padded = std::count(begin(encoded), end(encoded), '=');
    std::replace(begin(unpadded), end(unpadded), '=', 'A'); // A_64 == \0

    std::string decoded{detail::BinaryFromBase64{begin(unpadded)},
                        detail::BinaryFromBase64{begin(unpadded) + unpadded.length()}};

    decoded.erase(end(decoded) - num_padded, end(decoded));
    std::copy(begin(decoded), end(decoded), out);
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
#if !defined(__GNUC__) || (__GNUC__ > 4)
    static_assert(std::is_trivially_copyable<T>::value, "requires a trivially copyable type");
#endif

    T x;

    decodeBase64(encoded, reinterpret_cast<unsigned char *>(&x));

    return x;
}

} // namespace engine
} // namespace osrm

#endif /* OSRM_BASE64_HPP */
