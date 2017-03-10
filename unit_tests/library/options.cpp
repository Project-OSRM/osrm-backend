#include <boost/test/test_case_template.hpp>
#include <boost/test/unit_test.hpp>

#include "coordinates.hpp"
#include "equal_json.hpp"
#include "fixture.hpp"

#include "osrm/osrm.hpp"

BOOST_AUTO_TEST_SUITE(options)

BOOST_AUTO_TEST_CASE(test_ch)
{
    using namespace osrm;
    EngineConfig config;
    config.use_shared_memory = false;
    config.storage_config = storage::StorageConfig(OSRM_TEST_DATA_DIR "/monaco_CH.osrm");
    config.algorithm = EngineConfig::Algorithm::CH;
    OSRM osrm{config};
}

BOOST_AUTO_TEST_CASE(test_corech)
{
    using namespace osrm;
    EngineConfig config;
    config.use_shared_memory = false;
    config.storage_config = storage::StorageConfig(OSRM_TEST_DATA_DIR "/monaco_CoreCH.osrm");
    config.algorithm = EngineConfig::Algorithm::CoreCH;
    OSRM osrm{config};
}

BOOST_AUTO_TEST_CASE(test_mld)
{
    using namespace osrm;
    EngineConfig config;
    config.use_shared_memory = false;
    config.storage_config = storage::StorageConfig(OSRM_TEST_DATA_DIR "/monaco_MLD.osrm");
    config.algorithm = EngineConfig::Algorithm::MLD;
    OSRM osrm{config};
}

BOOST_AUTO_TEST_SUITE_END()
