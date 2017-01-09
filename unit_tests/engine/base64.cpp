#include "engine/base64.hpp"
#include "mocks/mock_datafacade.hpp"
#include "engine/hint.hpp"

#include <boost/test/test_case_template.hpp>
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
    const osrm::test::MockDataFacade<osrm::engine::algorithm::CH> facade{};

    const Hint hint{phantom, facade.GetCheckSum()};

    const auto base64 = hint.ToBase64();

    BOOST_CHECK(0 == std::count(begin(base64), end(base64), '+'));
    BOOST_CHECK(0 == std::count(begin(base64), end(base64), '/'));

    const auto decoded = Hint::FromBase64(base64);

    BOOST_CHECK_EQUAL(hint, decoded);
}

BOOST_AUTO_TEST_CASE(hint_encoding_decoding_roundtrip_bytewise)
{
    using namespace osrm::engine;
    using namespace osrm::util;

    const Coordinate coordinate;
    const PhantomNode phantom;
    const osrm::test::MockDataFacade<osrm::engine::algorithm::CH> facade{};

    const Hint hint{phantom, facade.GetCheckSum()};

    const auto decoded = Hint::FromBase64(hint.ToBase64());

    BOOST_CHECK(std::equal(reinterpret_cast<const unsigned char *>(&hint),
                           reinterpret_cast<const unsigned char *>(&hint) + sizeof(Hint),
                           reinterpret_cast<const unsigned char *>(&decoded)));
}

BOOST_AUTO_TEST_SUITE_END()
