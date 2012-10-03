@routing @bicycle @access
Feature: Bike - Restricted access

	Background:
		Given the speedprofile "bicycle"
		
	Scenario: Bike - Access tags on ways		
	 	Then routability should be
		 | access        | bothw |
		 |               | x     |
		 | yes           | x     |
		 | motorcar      | x     |
		 | motor_vehicle | x     |
		 | vehicle       | x     |
		 | permissive    | x     |
		 | designated    | x     |
		 | no            |       |
		 | foot          |       |
		 | private       |       |
		 | agricultural  |       |
		 | forestery     |       |
		 | some_tag      | x     |


	Scenario: Bike - Access tags on nodes		
	 	Then routability should be
		 | node/access   | bothw |
		 |               | x     |
		 | yes           | x     |
		 | motorcar      | x     |
		 | motor_vehicle | x     |
		 | vehicle       | x     |
		 | permissive    | x     |
		 | designated    | x     |
		 | no            |       |
		 | foot          |       |
		 | private       |       |
		 | agricultural  |       |
		 | forestery     |       |
		 | some_tag      | x     |

	Scenario: Bike - Access tags on both nodes and way 		
	 	Then routability should be
		 | access   | node/access | bothw |
		 | yes      | yes         | x     |
		 | yes      | no          |       |
		 | yes      | foot        |       |
		 | yes      | some_tag    | x     |
		 | no       | yes         |       |
		 | no       | no          |       |
		 | no       | foot        |       |
		 | no       | some_tag    |       |
		 | some_tag | yes         | x     |
		 | some_tag | no          |       |
		 | some_tag | foot        |       |
		 | some_tag | some_tag    | x     |
