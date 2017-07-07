#ifndef OSRM_LIBRARY_TEST_FIXTURE
#define OSRM_LIBRARY_TEST_FIXTURE

#include "osrm/engine_config.hpp"
#include "osrm/osrm.hpp"

#include <string>

// I couldn't get Boost.UnitTest to provide a test suite level fixture with custom
// arguments per test suite (osrm base path from argv), so this has to suffice.

inline osrm::OSRM
getOSRM(const std::string &base_path,
        osrm::EngineConfig::Algorithm algorithm = osrm::EngineConfig::Algorithm::CH)
{
    osrm::EngineConfig config;
    config.storage_config = {base_path};
    config.use_shared_memory = false;
    config.algorithm = algorithm;

    return osrm::OSRM{config};
}

#endif
