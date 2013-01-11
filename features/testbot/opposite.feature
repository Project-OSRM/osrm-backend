@routing @opposite @todo
Feature: Separate settings for forward/backward direction
	
	Background:
		Given the profile "testbot"
	
	@smallest
	Scenario: Going against the flow
		Given the node map
		 | a | b |
	
		And the ways
		 | nodes | highway |
		 | ab    | river   |
    
		When I route I should get
		 | from | to | route | distance | time |
		 | a    | b  | ab    | 100m     | 10s  |
		 | b    | a  | ab    | 100m     | 20s  |
