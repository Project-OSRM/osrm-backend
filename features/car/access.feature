@routing @car @access
Feature: Car - Restricted access

	Background:
		Given the speedprofile "car"
		
	Scenario: Car - Access tags on ways		
	 	Then routability should be
		 | access        | bothw |
		 | yes           | x     |
		 | motorcar      | x     |
		 | motor_vehicle | x     |
		 | vehicle       | x     |
		 | permissive    | x     |
		 | designated    | x     |
		 | no            |       |
		 | private       |       |
		 | agricultural  |       |
		 | forestery     |       |
		 | some_tag      | x     |


	Scenario: Car - Access tags on nodes		
	 	Then routability should be
		 | node/access   | bothw |
		 | yes           | x     |
		 | motorcar      | x     |
		 | motor_vehicle | x     |
		 | vehicle       | x     |
		 | permissive    | x     |
		 | designated    | x     |
		 | no            |       |
		 | private       |       |
		 | agricultural  |       |
		 | forestery     |       |
		 | some_tag      | x     |

	Scenario: Car - Access tags on both nodes and way 		
	 	Then routability should be
		 | access   | node/access | bothw |
		 | yes      | yes         | x     |
		 | yes      | no          |       |
		 | yes      | some_tag    | x     |
		 | no       | yes         |       |
		 | no       | no          |       |
		 | no       | some_tag    |       |
		 | some_tag | yes         | x     |
		 | some_tag | no          |       |
		 | some_tag | some_tag    | x     |