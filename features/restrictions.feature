@routing @restrictions
Feature: Turn restrictions
	OSRM should handle turn restrictions

Scenario: No left turn when crossing a oneway street
    When I request a route from 55.689741159238,12.574720202639 to 55.689741159232,12.57455927015
    Then I should get a route
 	And the route should start at "Sølvgade"
	And the route should end at "Øster Farimagsgade"
 	And the route should not include "Sølvgade, Øster Farimagsgade"
	And no error should be reported in terminal

Scenario: No left turn at T-junction: Don't turn left from side road into main road
    When I request a route from 55.66442995717,12.549384056343 to 55.664060815164,12.548944174065
    Then I should get a route
 	And the route should start at "Sigerstedgade"
	And the route should end at "Ingerslevsgade"
	And there should be more than 1 turn
 	And the route should not include "Sigerstedgade, Ingerslevsgade"
    
Scenario: No left turn at T-junction: OK to turn right from side road into main road
	When I request a route from 55.66442995717,12.549384056343 to 55.664218154805,12.5502638209
    Then I should get a route
 	And the route should start at "Sigerstedgade"
	And the route should end at "Ingerslevsgade"
	And there should be 1 turn
 	And the route should include "Sigerstedgade, Ingerslevsgade"

Scenario: No left turn at T-junction: OK to go straight on main road
    When I request a route from 55.66419092299,12.550333558335 to 55.664060815164,12.548944174065
    Then I should get a route
 	And the route should stay on "Ingerslevsgade"

Scenario: No left turn at T-junction: OK to turn right from main road into side road
    When I request a route from 55.664060815164,12.548944174065 to 55.66442995717,12.549384056343 
    Then I should get a route
	And the route should start at "Ingerslevsgade"
 	And the route should end at "Sigerstedgade"
 	And there should be 1 turn
	And the route should include "Ingerslevsgade, Sigerstedgade"

Scenario: No left turn at T-junction: OK to turn left from main road into side road
    When I request a route from 55.664218154805,12.5502638209 to 55.66442995717,12.549384056343 
    Then I should get a route
	And the route should start at "Ingerslevsgade"
 	And the route should end at "Sigerstedgade"
 	And there should be 1 turn
	And the route should include "Ingerslevsgade, Sigerstedgade"

