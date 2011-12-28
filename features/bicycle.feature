@routing @bicycle
Feature: Bicycle Routing from A to B
	To enable bicycle routing
	OSRM should handle all relevant bicycle tags

 	Scenario: oneway:bicycle
		When I request a route from 55.673168935147,12.563557740441 to 55.67380116846,12.563107129324&
		Then I should get a route
		And the route should follow "Banegårdspladsen"
		And there should not be any turns
		And the distance should be close to 80m

 	Scenario: cycleway=opposite_lane
		When I request a route from 55.689126237262,12.553137305887 to 55.688666612359,12.55296564451
		Then I should get a route
		And the route should follow "Kapelvej"
		And there should not be any turns
		And the distance should be close to 50m


