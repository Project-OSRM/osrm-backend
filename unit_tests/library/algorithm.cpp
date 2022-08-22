#include <boost/test/unit_test.hpp>

#include "fixture.hpp"

#include "osrm/exception.hpp"

BOOST_AUTO_TEST_SUITE(table)

BOOST_AUTO_TEST_CASE(test_incompatible_with_mld)
{
    // Can't use the MLD algorithm with CH data
    BOOST_CHECK_THROW(
        getOSRM(OSRM_TEST_DATA_DIR "/ch/monaco.osrm", osrm::EngineConfig::Algorithm::MLD),
        osrm::exception);
}

BOOST_AUTO_TEST_CASE(test_compatible_with_corech_fallback)
{
    // Note - this tests that given the CoreCH algorithm config option, configuration falls back to
    // CH and is compatible with CH data
    BOOST_CHECK_NO_THROW(
        getOSRM(OSRM_TEST_DATA_DIR "/ch/monaco.osrm", osrm::EngineConfig::Algorithm::CoreCH));
}

BOOST_AUTO_TEST_CASE(test_incompatible_with_ch)
{
    // Can't use the CH algorithm with MLD data
    BOOST_CHECK_THROW(
        getOSRM(OSRM_TEST_DATA_DIR "/mld/monaco.osrm", osrm::EngineConfig::Algorithm::CH),
        osrm::exception);
}

BOOST_AUTO_TEST_SUITE_END()
