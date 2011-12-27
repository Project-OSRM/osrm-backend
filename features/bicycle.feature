@routing @bicycle
Feature: Bicycle Routing from A to B
	To enable bicycle routing
	OSRM should handle all relevant bicycle tags

 	Scenario: Respect oneway without additional tags

 	Scenario: oneway:bicycle
		When I request a route from 55.673168935147,12.563557740441 to 55.67380116846,12.563107129324&
		Then I should get a route
		And the route should follow "Banegårdspladsen"
		And there should not be any turns
		And the distance should be close to 70m

 	Scenario: cycleway=opposite_lane
		When I request a route from 55.689236488,12.55317804955 to 55.688510764046,12.552909828648
		Then I should get a route
		And the route should follow "Kapelvej"
		And there should not be any turns
		And the distance should be close to 80m


