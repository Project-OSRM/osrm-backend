@other
Feature: Other stuff

Scenario: No left turn when crossing a oneway street
    When I request a route from 55.689741159238,12.574720202639 to 55.689741159232,12.57455927015
    Then I should get a route
 	And the route should start at "Sølvgade"
	And the route should end at "Øster Farimagsgade"
 	And the route should not include "Sølvgade, Øster Farimagsgade"
	And no error should be reported in terminal

Scenario: No left turn at T-junction: Don't turn left from side road into main road
	When I request a route from 55.66442995717,12.549384056343 to 55.664218154805,12.5502638209
    Then I should get a route
 	And the route should start at "Sigerstedgade"
	And the route should end at "Ingerslevsgade"
 	And the route should not include "Sigerstedgade, Ingerslevsgade"
	And there should be more than 1 turn
    
Scenario: No left turn at T-junction: OK to turn right from side road into main road
	When I request a route from 55.66442995717,12.549384056343 to 55.664060815164,12.548944174065
    Then I should get a route
 	And the route should start at "Sigerstedgade"
	And the route should end at "Ingerslevsgade"
 	And the route should include "Sigerstedgade, Ingerslevsgade"
	And there should be 1 turn

Scenario: No left turn at T-junction: OK to go straight on main road
    When I request a route from 55.66419092299,12.550333558335 to 55.664060815164,12.548944174065
    Then I should get a route
 	And the route should stay on "Ingerslevsgade"

Scenario: No left turn at T-junction: OK to turn right from main road into side road
    When I request a route from 55.664060815164,12.548944174065 to 55.66442995717,12.549384056343 
    Then I should get a route
	And the route should start at "Ingerslevsgade"
 	And the route should end at "Sigerstedgade"
	And the route should include "Ingerslevsgade, Sigerstedgade"
 	And there should be 1 turn

Scenario: No left turn at T-junction: OK to turn left from main road into side road
    When I request a route from 55.664218154805,12.5502638209 to 55.66442995717,12.549384056343 
    Then I should get a route
	And the route should start at "Ingerslevsgade"
 	And the route should end at "Sigerstedgade"
	And the route should include "Ingerslevsgade, Sigerstedgade"
 	And there should be 1 turn



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


