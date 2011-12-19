@routing
Feature: Routing from A to B
	In order to make it easier to navigate
	As someone using the public road network
	I want to be able to use OSRM to compute routes based on OpenStreetMap data
			
	Scenario: Phantom shortcut
	    When I request a route from 55.662740149207,12.576105114488& to 55.665753800212,12.575547215013
	    Then I should get a route
	 	And the distance should be close to 450m

	Scenario: Start and stop markers should snap to closest streets
	    When I request a route from 55.6634,12.5724 to 55.6649,12.5742
	    Then I should get a route
	    And the route should stay on "Islands Brygge"
	 	And the distance should be close to 200m

	Scenario: Crossing round-about at Amalienborg
	    When I request a route from 55.683797649183,12.593940686704 to 55.6842149924,12.592476200581
	    Then I should get a route
	 	And the distance should be close to 150m
		
	@bicycle
	Scenario: Handle cycleway=opposite_lane
	    When I request a route from 55.689236488,12.55317804955 to 55.688510764046,12.552909828648
	    Then I should get a route
	 	And the route should follow "Kapelvej"
	 	And there should not be any turns
	 	And the distance should be close to 80m

	@bicycle
	Scenario: Handle oneway:bicycle
	    When I request a route from 55.673168935147,12.563557740441 to 55.67380116846,12.563107129324&
	    Then I should get a route
	 	And the route should follow "Banegårdspladsen"
	 	And there should not be any turns
	 	And the distance should be close to 70m

	Scenario: Requesting invalid routes
	    When I request a route from 0,0 to 0,0
	    Then I should not get a route
		And no error should be reported in terminal

	Scenario: Dont flicker
	    When I request a route from 55.658833555366,12.592788454378 to 55.663871808364,12.583497282355
	    Then I should get a route
	 	And the route should follow "Amagerfælledvej, Njalsgade, Artillerivej"
		And no error should be reported in terminal
	    When I request a route from 55.658821450674,12.592466589296 to 55.663871808364,12.583497282355
	    Then I should get a route
	 	And the route should follow "Kaj Munks Vej, Tom Kristensens Vej, Ørestads Boulevard, Njalsgade, Artillerivej"
		And no error should be reported in terminal
	    When I request a route from 55.658857764739,12.592058893525 to 55.663871808364,12.583497282355
	    Then I should get a route
	 	And the route should follow "Kaj Munks Vej, Tom Kristensens Vej, Ørestads Boulevard, Njalsgade, Artillerivej"
		And no error should be reported in terminal
		