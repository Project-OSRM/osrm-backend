#include "engine/base64.hpp"

#include <boost/test/unit_test.hpp>
#include <boost/test/test_case_template.hpp>

// RFC 4648 "The Base16, Base32, and Base64 Data Encodings"
BOOST_AUTO_TEST_SUITE(base64)

// For test vectors see section 10: https://tools.ietf.org/html/rfc4648#section-10
BOOST_AUTO_TEST_CASE(rfc4648_test_vectors)
{
    using namespace osrm::engine;

    BOOST_CHECK_EQUAL(encodeBase64(""), "");
    BOOST_CHECK_EQUAL(encodeBase64("f"), "Zg==");
    BOOST_CHECK_EQUAL(encodeBase64("fo"), "Zm8=");
    BOOST_CHECK_EQUAL(encodeBase64("foo"), "Zm9v");
    BOOST_CHECK_EQUAL(encodeBase64("foob"), "Zm9vYg==");
    BOOST_CHECK_EQUAL(encodeBase64("fooba"), "Zm9vYmE=");
    BOOST_CHECK_EQUAL(encodeBase64("foobar"), "Zm9vYmFy");
}

BOOST_AUTO_TEST_CASE(encoding) {}

BOOST_AUTO_TEST_CASE(decoding) {}

BOOST_AUTO_TEST_SUITE_END()
