@routing @car @barrier
Feature: Car - Barriers

	Background:
		Given the speedprofile "car"

	Scenario: Car - Barriers 
		Then routability should be
		 | node/barrier   | bothw |
		 |                | x     |
		 | bollard        |       |
		 | gate           | x     |
		 | cattle_grid    | x     |
		 | border_control | x     |
		 | toll_booth     | x     |
		 | sally_port     | x     |
		 | entrance       |       |
		 | wall           |       |
		 | fence          |       |
		 | some_tag       |       |

	Scenario: Car - Access tag trumphs barriers
		Then routability should be
		 | node/barrier | node/access   | bothw |
		 | gate         |               | x     |
		 | gate         | yes           | x     |
		 | gate         | vehicle       | x     |
		 | gate         | motorcar      | x     |
		 | gate         | motor_vehicle | x     |
		 | gate         | permissive    | x     |
		 | gate         | designated    | x     |
		 | gate         | no            |       |
		 | gate         | foot          |       |
		 | gate         | bicycle       |       |
		 | gate         | private       |       |
		 | gate         | agricultural  |       |
		 | wall         |               |       |
		 | wall         | yes           | x     |
		 | wall         | vehicle       | x     |
		 | wall         | motorcar      | x     |
		 | wall         | motor_vehicle | x     |
		 | wall         | permissive    | x     |
		 | wall         | designated    | x     |
		 | wall         | no            |       |
		 | wall         | foot          |       |
		 | wall         | bicycle       |       |
		 | wall         | private       |       |
		 | wall         | agricultural  |       |
