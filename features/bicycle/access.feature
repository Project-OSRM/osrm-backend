@routing @bicycle @access
Feature: Bike - Restricted access
Reference: http://wiki.openstreetmap.org/wiki/Key:access

	Background:
		Given the speedprofile "bicycle"
		
	Scenario: Bike - Access tag hierachy
		Then routability should be
		 | access | vehicle | bicycle | bothw |
		 | yes    |         |         | x     |
		 | no     |         |         |       |
		 | yes    | yes     |         | x     |
		 | no     | yes     |         | x     |
		 | yes    | no      |         |       |
		 | no     | no      |         |       |
		 | yes    | yes     | yes     | x     |
		 | no     | yes     | yes     | x     |
		 | yes    | no      | yes     | x     |
		 | no     | no      | yes     | x     |
		 | yes    | yes     | no      |       |
		 | no     | yes     | no      |       |
		 | yes    | no      | no      |       |
		 | no     | no      | no      |       |

	Scenario: Bike - Access tags on ways
	 	Then routability should be
		 | access        | bothw |
		 |               | x     |
		 | yes           | x     |
		 | permissive    | x     |
		 | designated    | x     |
		 | no            |       |
		 | private       |       |
		 | agricultural  |       |
		 | forestery     |       |
		 | some_tag      | x     |

	Scenario: Bike - Access tags on nodes
	 	Then routability should be
		 | node/access   | bothw |
		 |               | x     |
		 | yes           | x     |
		 | permissive    | x     |
		 | designated    | x     |
		 | no            |       |
		 | private       |       |
		 | agricultural  |       |
		 | forestery     |       |
		 | some_tag      | x     |

	Scenario: Bike - Access tags on both node and way
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
