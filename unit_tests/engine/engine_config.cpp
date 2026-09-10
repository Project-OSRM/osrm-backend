#include "engine/engine_config.hpp"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(engine_config)

BOOST_AUTO_TEST_CASE(requires_positive_isochrone_limits)
{
    osrm::engine::EngineConfig config;
    BOOST_REQUIRE(config.IsValid());

    config.max_isochrone_search_records = 0;
    BOOST_CHECK(!config.IsValid());
    config.max_isochrone_search_records = 1;

    config.max_isochrone_materialized_points = 0;
    BOOST_CHECK(!config.IsValid());
    config.max_isochrone_materialized_points = 1;

    config.max_isochrone_rasterization_steps = 0;
    BOOST_CHECK(!config.IsValid());
    config.max_isochrone_rasterization_steps = 1;

    config.max_isochrone_output_points = 0;
    BOOST_CHECK(!config.IsValid());
    config.max_isochrone_output_points = 1;

    config.max_isochrone_grid_cells = 0;
    BOOST_CHECK(!config.IsValid());
    config.max_isochrone_grid_cells = 1;

    config.max_isochrone_contours = 0;
    BOOST_CHECK(!config.IsValid());
    config.max_isochrone_contours = 1;

    BOOST_CHECK(config.IsValid());
}

BOOST_AUTO_TEST_SUITE_END()
