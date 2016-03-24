#include <boost/test/unit_test.hpp>
#include <boost/test/test_case_template.hpp>

#include "args.hpp"
#include "fixture.hpp"

#include "osrm/tile_parameters.hpp"

#include "osrm/coordinate.hpp"
#include "osrm/engine_config.hpp"
#include "osrm/json_container.hpp"
#include "osrm/status.hpp"
#include "osrm/osrm.hpp"

BOOST_AUTO_TEST_SUITE(tile)

BOOST_AUTO_TEST_CASE(test_tile)
{
    const auto args = get_args();
    auto osrm = getOSRM(args.at(0));

    using namespace osrm;

    TileParameters params{0, 0, 1};

    std::string result;
    const auto rc = osrm.Tile(params, result);
    BOOST_CHECK(rc == Status::Ok);
}

BOOST_AUTO_TEST_SUITE_END()
