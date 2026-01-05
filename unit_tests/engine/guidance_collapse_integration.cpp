#include "engine/guidance/collapse_turns.hpp"
#include "engine/guidance/collapsing_utility.hpp"
#include "../mocks/mock_datafacade.hpp"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(guidance_collapse_integration)

using namespace osrm::engine::guidance;
using namespace osrm::guidance;
using namespace osrm::extractor;

// Custom mock datafacade that allows setting max_collapse_distance
class ConfigurableMaxCollapseDistanceFacade : public osrm::test::MockBaseDataFacade
{
private:
    double max_collapse_distance_;

public:
    ConfigurableMaxCollapseDistanceFacade(double max_collapse_distance = 30.0) 
        : max_collapse_distance_(max_collapse_distance) {}
    
    double GetMaxCollapseDistance() const override { return max_collapse_distance_; }
    
    void SetMaxCollapseDistance(double value) { max_collapse_distance_ = value; }
};

BOOST_AUTO_TEST_CASE(facade_integration_default_value)
{
    ConfigurableMaxCollapseDistanceFacade facade;
    
    // Test that facade returns default value
    BOOST_CHECK_EQUAL(facade.GetMaxCollapseDistance(), 30.0);
}

BOOST_AUTO_TEST_CASE(facade_integration_custom_value)
{
    ConfigurableMaxCollapseDistanceFacade facade(15.5);
    
    // Test that facade returns custom value
    BOOST_CHECK_EQUAL(facade.GetMaxCollapseDistance(), 15.5);
    
    // Test that value can be changed
    facade.SetMaxCollapseDistance(42.0);
    BOOST_CHECK_EQUAL(facade.GetMaxCollapseDistance(), 42.0);
}

BOOST_AUTO_TEST_CASE(collapse_turns_sets_thread_local_variable)
{
    // Store original value to restore later
    const double original_value = current_max_collapse_distance;
    
    ConfigurableMaxCollapseDistanceFacade facade(25.5);
    
    // Create minimal route steps for testing
    std::vector<RouteStep> steps;
    
    // Create depart step (required by collapseTurnInstructions)
    RouteStep depart_step;
    depart_step.maneuver.waypoint_type = WaypointType::Depart;
    depart_step.maneuver.instruction = {TurnType::NoTurn, DirectionModifier::UTurn};
    steps.push_back(depart_step);
    
    // Create arrive step (required by collapseTurnInstructions)
    RouteStep arrive_step;
    arrive_step.maneuver.waypoint_type = WaypointType::Arrive;
    arrive_step.maneuver.instruction = {TurnType::NoTurn, DirectionModifier::UTurn};
    steps.push_back(arrive_step);
    
    // Call collapseTurnInstructions which should set the thread-local variable
    auto result_steps = collapseTurnInstructions(facade, std::move(steps));
    
    // Verify that the thread-local variable was set correctly
    BOOST_CHECK_EQUAL(current_max_collapse_distance, 25.5);
    
    // Test with different value
    facade.SetMaxCollapseDistance(12.3);
    
    // Create new steps for second test
    std::vector<RouteStep> steps2;
    steps2.push_back(depart_step);
    steps2.push_back(arrive_step);
    
    auto result_steps2 = collapseTurnInstructions(facade, std::move(steps2));
    
    // Verify the thread-local variable was updated
    BOOST_CHECK_EQUAL(current_max_collapse_distance, 12.3);
    
    // Restore original value
    current_max_collapse_distance = original_value;
}

BOOST_AUTO_TEST_CASE(multiple_facades_independence)
{
    // Store original value to restore later
    const double original_value = current_max_collapse_distance;
    
    ConfigurableMaxCollapseDistanceFacade facade1(10.0);
    ConfigurableMaxCollapseDistanceFacade facade2(20.0);
    
    // Test that facades are independent
    BOOST_CHECK_EQUAL(facade1.GetMaxCollapseDistance(), 10.0);
    BOOST_CHECK_EQUAL(facade2.GetMaxCollapseDistance(), 20.0);
    
    // Create minimal route steps
    std::vector<RouteStep> steps1, steps2;
    RouteStep depart_step, arrive_step;
    depart_step.maneuver.waypoint_type = WaypointType::Depart;
    depart_step.maneuver.instruction = {TurnType::NoTurn, DirectionModifier::UTurn};
    arrive_step.maneuver.waypoint_type = WaypointType::Arrive;
    arrive_step.maneuver.instruction = {TurnType::NoTurn, DirectionModifier::UTurn};
    
    steps1.push_back(depart_step);
    steps1.push_back(arrive_step);
    steps2.push_back(depart_step);
    steps2.push_back(arrive_step);
    
    // Use facade1
    auto result1 = collapseTurnInstructions(facade1, std::move(steps1));
    BOOST_CHECK_EQUAL(current_max_collapse_distance, 10.0);
    
    // Use facade2
    auto result2 = collapseTurnInstructions(facade2, std::move(steps2));
    BOOST_CHECK_EQUAL(current_max_collapse_distance, 20.0);
    
    // Restore original value
    current_max_collapse_distance = original_value;
}

BOOST_AUTO_TEST_SUITE_END()
