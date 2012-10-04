@routing @car @access
Feature: Car - Restricted access
Reference: http://wiki.openstreetmap.org/wiki/Key:access

	Background:
		Given the speedprofile "car"
		
	Scenario: Car - Access tag hierachy		
		Then routability should be
		 | access | vehicle | motor_vehicle | motorcar | bothw |
		 |        |         |               |          | x     |
		 | yes    |         |               |          | x     |
		 | no     |         |               |          |       |
		 |        | yes     |               |          | x     |
		 |        | no      |               |          |       |
		 | no     | yes     |               |          | x     |
		 | yes    | no      |               |          |       |
		 |        |         | yes           |          | x     |
		 |        |         | no            |          |       |
		 | no     |         | yes           |          | x     |
		 | yes    |         | no            |          |       |
		 |        | no      | yes           |          | x     |
		 |        | yes     | no            |          |       |
		 |        |         |               | yes      | x     |
		 |        |         |               | no       |       |
		 | no     |         |               | yes      | x     |
		 | yes    |         |               | no       |       |
		 |        | no      |               | yes      | x     |
		 |        | yes     |               | no       |       |
		 |        |         | no            | yes      | x     |
		 |        |         | yes           | no       |       |

	Scenario: Car - Overwriting implied acccess 
		Then routability should be
		 | highway | access | vehicle | motor_vehicle | motorcar | bothw |
		 | primary |        |         |               |          | x     |
		 | runway  |        |         |               |          |       |
		 | primary | no     |         |               |          |       |
		 | primary |        | no      |               |          |       |
		 | primary |        |         | no            |          |       |
		 | primary |        |         |               | no       |       |
		 | runway  | yes    |         |               |          | x     |
		 | runway  |        | yes     |               |          | x     |
		 | runway  |        |         | yes           |          | x     |
		 | runway  |        |         |               | yes      | x     |

	Scenario: Car - Access tags on ways
	 	Then routability should be
		 | access       | bothw |
		 | yes          | x     |
		 | permissive   | x     |
		 | designated   | x     |
		 | no           |       |
		 | private      |       |
		 | agricultural |       |
		 | forestery    |       |
		 | some_tag     | x     |


	Scenario: Car - Access tags on nodes
	 	Then routability should be
		 | node/access  | bothw |
		 | yes          | x     |
		 | permissive   | x     |
		 | designated   | x     |
		 | no           |       |
		 | private      |       |
		 | agricultural |       |
		 | forestery    |       |
		 | some_tag     | x     |

	Scenario: Car - Access tags on both node and way
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
