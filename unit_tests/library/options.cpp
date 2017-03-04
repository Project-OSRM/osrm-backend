#include <boost/test/test_case_template.hpp>
#include <boost/test/unit_test.hpp>

#include "args.hpp"
#include "coordinates.hpp"
#include "equal_json.hpp"
#include "fixture.hpp"

#include "osrm/osrm.hpp"

BOOST_AUTO_TEST_SUITE(options)

BOOST_AUTO_TEST_CASE(test_ch)
{
    const auto args = get_args();

    using namespace osrm;
    EngineConfig config;
    config.storage_config = storage::StorageConfig(args.at(0));
    config.algorithm = EngineConfig::Algorithm::CH;
    OSRM osrm {config};
}

BOOST_AUTO_TEST_CASE(test_corech)
{
    const auto args = get_args();

    using namespace osrm;
    EngineConfig config;
    config.storage_config = storage::StorageConfig(args.at(0));
    config.algorithm = EngineConfig::Algorithm::CoreCH;
    OSRM osrm {config};
}

BOOST_AUTO_TEST_CASE(test_mld)
{
    const auto args = get_args();

    using namespace osrm;
    EngineConfig config;
    config.storage_config = storage::StorageConfig(args.at(0));
    config.algorithm = EngineConfig::Algorithm::MLD;
    OSRM osrm {config};
}


BOOST_AUTO_TEST_SUITE_END()
