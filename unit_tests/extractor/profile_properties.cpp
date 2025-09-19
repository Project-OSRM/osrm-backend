#include "extractor/profile_properties.hpp"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(profile_properties)

using namespace osrm::extractor;

BOOST_AUTO_TEST_CASE(default_max_collapse_distance)
{
    ProfileProperties properties;
    
    // Test that default max_collapse_distance is 30.0 meters
    BOOST_CHECK_EQUAL(properties.GetMaxCollapseDistance(), 30.0);
}

BOOST_AUTO_TEST_CASE(set_get_max_collapse_distance)
{
    ProfileProperties properties;
    
    // Test setting and getting custom values
    properties.SetMaxCollapseDistance(15.5);
    BOOST_CHECK_EQUAL(properties.GetMaxCollapseDistance(), 15.5);
    
    // Test setting to zero (edge case)
    properties.SetMaxCollapseDistance(0.0);
    BOOST_CHECK_EQUAL(properties.GetMaxCollapseDistance(), 0.0);
    
    // Test setting to large value
    properties.SetMaxCollapseDistance(1000.0);
    BOOST_CHECK_EQUAL(properties.GetMaxCollapseDistance(), 1000.0);
    
    // Test setting to small positive value
    properties.SetMaxCollapseDistance(0.1);
    BOOST_CHECK_EQUAL(properties.GetMaxCollapseDistance(), 0.1);
}

BOOST_AUTO_TEST_CASE(max_collapse_distance_independence)
{
    ProfileProperties properties1;
    ProfileProperties properties2;
    
    // Test that different instances are independent
    properties1.SetMaxCollapseDistance(10.0);
    properties2.SetMaxCollapseDistance(25.0);
    
    BOOST_CHECK_EQUAL(properties1.GetMaxCollapseDistance(), 10.0);
    BOOST_CHECK_EQUAL(properties2.GetMaxCollapseDistance(), 25.0);
    
    // Modify one and check the other is unchanged
    properties1.SetMaxCollapseDistance(50.0);
    BOOST_CHECK_EQUAL(properties1.GetMaxCollapseDistance(), 50.0);
    BOOST_CHECK_EQUAL(properties2.GetMaxCollapseDistance(), 25.0);
}

BOOST_AUTO_TEST_CASE(copy_constructor_preserves_max_collapse_distance)
{
    ProfileProperties original;
    original.SetMaxCollapseDistance(42.5);
    
    // Test copy constructor
    ProfileProperties copy = original;
    BOOST_CHECK_EQUAL(copy.GetMaxCollapseDistance(), 42.5);
    
    // Test that modifying copy doesn't affect original
    copy.SetMaxCollapseDistance(100.0);
    BOOST_CHECK_EQUAL(original.GetMaxCollapseDistance(), 42.5);
    BOOST_CHECK_EQUAL(copy.GetMaxCollapseDistance(), 100.0);
}

BOOST_AUTO_TEST_CASE(assignment_operator_preserves_max_collapse_distance)
{
    ProfileProperties original;
    ProfileProperties assigned;
    
    original.SetMaxCollapseDistance(33.3);
    assigned.SetMaxCollapseDistance(66.6);
    
    // Test assignment operator
    assigned = original;
    BOOST_CHECK_EQUAL(assigned.GetMaxCollapseDistance(), 33.3);
    
    // Test that modifying assigned doesn't affect original
    assigned.SetMaxCollapseDistance(99.9);
    BOOST_CHECK_EQUAL(original.GetMaxCollapseDistance(), 33.3);
    BOOST_CHECK_EQUAL(assigned.GetMaxCollapseDistance(), 99.9);
}

BOOST_AUTO_TEST_SUITE_END()
