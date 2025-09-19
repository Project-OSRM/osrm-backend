#include "engine/guidance/collapsing_utility.hpp"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(guidance_collapse_distance)

using namespace osrm::engine::guidance;

BOOST_AUTO_TEST_CASE(default_max_collapse_distance_constant)
{
    // Test that the default constant is properly defined
    BOOST_CHECK_EQUAL(DEFAULT_MAX_COLLAPSE_DISTANCE, 30.0);
}

BOOST_AUTO_TEST_CASE(thread_local_max_collapse_distance_default)
{
    // Test that thread-local variable defaults to the constant
    BOOST_CHECK_EQUAL(current_max_collapse_distance, DEFAULT_MAX_COLLAPSE_DISTANCE);
}

BOOST_AUTO_TEST_CASE(thread_local_max_collapse_distance_modification)
{
    // Store original value to restore later
    const double original_value = current_max_collapse_distance;
    
    // Test setting custom values
    current_max_collapse_distance = 15.0;
    BOOST_CHECK_EQUAL(current_max_collapse_distance, 15.0);
    
    current_max_collapse_distance = 0.5;
    BOOST_CHECK_EQUAL(current_max_collapse_distance, 0.5);
    
    current_max_collapse_distance = 100.0;
    BOOST_CHECK_EQUAL(current_max_collapse_distance, 100.0);
    
    // Restore original value for other tests
    current_max_collapse_distance = original_value;
    BOOST_CHECK_EQUAL(current_max_collapse_distance, original_value);
}

BOOST_AUTO_TEST_CASE(thread_local_variable_precision)
{
    // Store original value to restore later
    const double original_value = current_max_collapse_distance;
    
    // Test that precise values are maintained
    current_max_collapse_distance = 12.345;
    BOOST_CHECK_EQUAL(current_max_collapse_distance, 12.345);
    
    current_max_collapse_distance = 0.001;
    BOOST_CHECK_EQUAL(current_max_collapse_distance, 0.001);
    
    // Restore original value
    current_max_collapse_distance = original_value;
}

BOOST_AUTO_TEST_SUITE_END()
