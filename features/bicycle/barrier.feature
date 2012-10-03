@routing @bicycle @barrier
Feature: Barriers

	Background:
		Given the speedprofile "bicycle"

	Scenario: Bike - Barriers 
		Then routability should be
		 | node/barrier   | bothw |
		 |                | x     |
		 | bollard        | x     |
		 | gate           | x     |
		 | cattle_grid    | x     |
		 | border_control | x     |
		 | toll_booth     | x     |
		 | sally_port     | x     |
		 | entrance       | x     |
		 | wall           |       |
		 | fence          |       |
		 | some_tag       |       |

	Scenario: Bike - Access tag trumphs barriers
		Then routability should be
		 | node/barrier | node/access   | bothw |
		 | bollard      |               | x     |
		 | bollard      | yes           | x     |
		 | bollard      | bicycle       | x     |
		 | bollard      | vehicle       | x     |
		 | bollard      | motorcar      | x     |
		 | bollard      | motor_vehicle | x     |
		 | bollard      | permissive    | x     |
		 | bollard      | designated    | x     |
		 | bollard      | no            |       |
		 | bollard      | foot          |       |
		 | bollard      | private       |       |
		 | bollard      | agricultural  |       |
		 | wall         |               |       |
		 | wall         | yes           | x     |
		 | wall         | bicycle       | x     |
		 | wall         | vehicle       | x     |
		 | wall         | motorcar      | x     |
		 | wall         | motor_vehicle | x     |
		 | wall         | permissive    | x     |
		 | wall         | designated    | x     |
		 | wall         | no            |       |
		 | wall         | foot          |       |
		 | wall         | private       |       |
		 | wall         | agricultural  |       |
