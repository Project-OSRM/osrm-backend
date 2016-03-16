#include <boost/test/unit_test.hpp>
#include <boost/test/test_case_template.hpp>

#include "args.hpp"

#include "osrm/match_parameters.hpp"

#include "osrm/coordinate.hpp"
#include "osrm/engine_config.hpp"
#include "osrm/json_container.hpp"
#include "osrm/status.hpp"
#include "osrm/osrm.hpp"

BOOST_AUTO_TEST_SUITE(match)

BOOST_AUTO_TEST_CASE(test_match)
{
    const auto args = get_args();
    BOOST_REQUIRE_EQUAL(args.size(), 1);

    using namespace osrm;

    EngineConfig config{args[0]};
    config.use_shared_memory = false;

    OSRM osrm{config};

    /*
    MatchParameters params;

    json::Object result;

    const auto rc = osrm.Match(params, result);

    BOOST_CHECK(rc == Status::Ok || rc == Status::Error);
    */
}

BOOST_AUTO_TEST_SUITE_END()
