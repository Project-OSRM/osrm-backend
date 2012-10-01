@routing @bicycle @access
Feature: Bike - Restricted access

	Background:
		Given the speedprofile "bicycle"
		
	Scenario: Bike - Access tags on ways		
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
		 | forestry      |       |
		 | designated    |       |
		 | some_tag      | x     |

	Scenario: Bike - Access tags for other modes should have no effect	
	 	Then routability should be
		 | access            | bothw |
		 | ufo:yes           | x     |
		 | ufo:motorcar      | x     |
		 | ufo:motor_vehicle | x     |
		 | ufo:vehicle       | x     |
		 | ufo:permissive    | x     |
		 | ufo:designated    | x     |
		 | ufo:no            | x     |
		 | ufo:private       | x     |
		 | ufo:agricultural  | x     |
		 | ufo:forestry      | x     |
		 | ufo:designated    | x     |
		 | ufo:some_tag      | x     |

	Scenario: Bike - Access tags on nodes		
	 	Then routability should be
		 | node:access   | bothw |
		 | yes           | x     |
		 | motorcar      | x     |
		 | motor_vehicle | x     |
		 | vehicle       | x     |
		 | permissive    | x     |
		 | designated    | x     |
		 | no            |       |
		 | private       |       |
		 | agricultural  |       |
		 | forestry      |       |
		 | designated    |       |
		 | some_tag      | x     |

	Scenario: Bike - Access tags on both nodes and way 		
	 	Then routability should be
		 | access   | node:access | bothw |
		 | yes      | yes         | x     |
		 | yes      | no          |       |
		 | yes      | some_tag    |       |
		 | no       | yes         |       |
		 | no       | no          |       |
		 | no       | some_tag    |       |
		 | some_tag | yes         |       |
		 | some_tag | no          |       |
		 | some_tag | some_tag    |       |
