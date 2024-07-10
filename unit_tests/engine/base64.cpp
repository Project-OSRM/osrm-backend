#include "engine/base64.hpp"
#include "mocks/mock_datafacade.hpp"
#include "engine/hint.hpp"

#include <boost/test/unit_test.hpp>

#include <algorithm>
#include <iostream>

// RFC 4648 "The Base16, Base32, and Base64 Data Encodings"
BOOST_AUTO_TEST_SUITE(base64)

// For test vectors see section 10: https://tools.ietf.org/html/rfc4648#section-10
BOOST_AUTO_TEST_CASE(rfc4648_test_vectors)
{
    using namespace osrm::engine;

    BOOST_CHECK_EQUAL(encodeBase64("f"), "Zg==");
    BOOST_CHECK_EQUAL(encodeBase64("fo"), "Zm8=");
    BOOST_CHECK_EQUAL(encodeBase64("foo"), "Zm9v");
    BOOST_CHECK_EQUAL(encodeBase64("foob"), "Zm9vYg==");
    BOOST_CHECK_EQUAL(encodeBase64("fooba"), "Zm9vYmE=");
    BOOST_CHECK_EQUAL(encodeBase64("foobar"), "Zm9vYmFy");
}

BOOST_AUTO_TEST_CASE(rfc4648_test_vectors_roundtrip)
{
    using namespace osrm::engine;

    BOOST_CHECK_EQUAL(decodeBase64(encodeBase64("f")), "f");
    BOOST_CHECK_EQUAL(decodeBase64(encodeBase64("fo")), "fo");
    BOOST_CHECK_EQUAL(decodeBase64(encodeBase64("foo")), "foo");
    BOOST_CHECK_EQUAL(decodeBase64(encodeBase64("foob")), "foob");
    BOOST_CHECK_EQUAL(decodeBase64(encodeBase64("fooba")), "fooba");
    BOOST_CHECK_EQUAL(decodeBase64(encodeBase64("foobar")), "foobar");
}

BOOST_AUTO_TEST_CASE(hint_encoding_decoding_roundtrip)
{
    using namespace osrm::engine;
    using namespace osrm::util;

    const Coordinate coordinate;
    const PhantomNode phantom;
    const osrm::test::MockDataFacade<osrm::engine::routing_algorithms::ch::Algorithm> facade{};

    const SegmentHint seg_hint{phantom, facade.GetCheckSum()};

    const auto base64 = seg_hint.ToBase64();

    BOOST_CHECK(0 == std::count(begin(base64), end(base64), '+'));
    BOOST_CHECK(0 == std::count(begin(base64), end(base64), '/'));

    const auto decoded = SegmentHint::FromBase64(base64);

    BOOST_CHECK_EQUAL(seg_hint, decoded);
}

BOOST_AUTO_TEST_CASE(hint_encoding_decoding_roundtrip_bytewise)
{
    using namespace osrm::engine;
    using namespace osrm::util;

    const Coordinate coordinate;
    const PhantomNode phantom;
    const osrm::test::MockDataFacade<osrm::engine::routing_algorithms::ch::Algorithm> facade{};

    const SegmentHint seg_hint{phantom, facade.GetCheckSum()};

    const auto decoded = SegmentHint::FromBase64(seg_hint.ToBase64());

    BOOST_CHECK(std::equal(reinterpret_cast<const unsigned char *>(&seg_hint),
                           reinterpret_cast<const unsigned char *>(&seg_hint) + sizeof(Hint),
                           reinterpret_cast<const unsigned char *>(&decoded)));
}

BOOST_AUTO_TEST_CASE(long_string_encoding)
{
    using namespace osrm::engine;
    std::string long_string(1000, 'A'); // String of 1000 'A's
    std::string encoded = encodeBase64(long_string);
    BOOST_CHECK_EQUAL(decodeBase64(encoded), long_string);
}

BOOST_AUTO_TEST_CASE(invalid_base64_decoding)
{
    using namespace osrm::engine;
    BOOST_CHECK_THROW(decodeBase64("Invalid!"), std::exception);
}

BOOST_AUTO_TEST_CASE(hint_serialization_size)
{
    using namespace osrm::engine;
    using namespace osrm::util;

    const Coordinate coordinate;
    const PhantomNode phantom;
    const osrm::test::MockDataFacade<osrm::engine::routing_algorithms::ch::Algorithm> facade{};

    const SegmentHint hint{phantom, facade.GetCheckSum()};
    const auto base64 = hint.ToBase64();

    BOOST_CHECK_EQUAL(base64.size(), 112);
}

BOOST_AUTO_TEST_CASE(extended_roundtrip_tests)
{
    using namespace osrm::engine;

    std::vector<std::string> test_strings = {
        "Hello, World!",                    // Simple ASCII string
        "1234567890",                       // Numeric string
        "!@#$%^&*()_+",                     // Special characters
        std::string(1000, 'A'),             // Long repeating string
        "¡Hola, mundo!",                    // Non-ASCII characters
        "こんにちは、世界！",               // Unicode characters
        std::string("\x00\x01\x02\x03", 4), // Binary data
        "a",                                // Single character
        "ab",                               // Two characters
        "abc",                              // Three characters (no padding in Base64)
        std::string(190, 'x')               // String that doesn't align with Base64 padding
    };

    for (const auto &test_str : test_strings)
    {
        std::string encoded = encodeBase64(test_str);
        std::string decoded = decodeBase64(encoded);
        BOOST_CHECK_EQUAL(decoded, test_str);

        // Additional checks
        BOOST_CHECK(encoded.find_first_not_of(
                        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=") ==
                    std::string::npos);
        if (test_str.length() % 3 != 0)
        {
            BOOST_CHECK(encoded.back() == '=');
        }
    }
}

BOOST_AUTO_TEST_CASE(roundtrip_with_url_safe_chars)
{
    using namespace osrm::engine;

    std::string original = "Hello+World/Nothing?Is:Impossible";
    std::string encoded = encodeBase64(original);

    // Replace '+' with '-' and '/' with '_'
    std::replace(encoded.begin(), encoded.end(), '+', '-');
    std::replace(encoded.begin(), encoded.end(), '/', '_');

    std::string decoded = decodeBase64(encoded);
    BOOST_CHECK_EQUAL(decoded, original);
}

BOOST_AUTO_TEST_CASE(roundtrip_stress_test)
{
    using namespace osrm::engine;

    std::string test_str;
    for (int i = 0; i < 1000; ++i)
    {
        test_str += static_cast<char>(i % 256);
    }

    std::string encoded = encodeBase64(test_str);
    std::string decoded = decodeBase64(encoded);
    BOOST_CHECK_EQUAL(decoded, test_str);
}

BOOST_AUTO_TEST_SUITE_END()
