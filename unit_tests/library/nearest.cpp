#include <boost/test/unit_test.hpp>
#include <boost/test/test_case_template.hpp>

#include "args.h"

#include "osrm/nearest_parameters.hpp"

#include "osrm/coordinate.hpp"
#include "osrm/engine_config.hpp"
#include "osrm/json_container.hpp"
#include "osrm/status.hpp"
#include "osrm/osrm.hpp"

BOOST_AUTO_TEST_SUITE(nearest)

BOOST_AUTO_TEST_CASE(test_nearest)
{
    const auto args = get_args();
    BOOST_REQUIRE_EQUAL(args.size(), 1);

    using namespace osrm;

    EngineConfig config{args[0]};
    config.use_shared_memory = false;

    OSRM osrm{config};

    /*
    NearestParameters params;

    json::Object result;

    const auto rc = osrm.Nearest(params, result);

    BOOST_CHECK(rc == Status::Ok || rc == Status::Error);
    */
}

BOOST_AUTO_TEST_SUITE_END()
