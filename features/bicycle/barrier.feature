@routing @bicycle @barrier
Feature: Barriers

	Background:
		Given the speedprofile "bicycle"

	Scenario: Barriers 
		Then routability should be
		 | highway | node/barrier   | node/access   | bothw |
		 | primary |                |               | x     |
		 | primary | gate           |               | x     |
		 | primary | cattle_grid    |               | x     |
		 | primary | border_control |               | x     |
		 | primary | toll_booth     |               | x     |
		 | primary | sally_port     |               | x     |
		 | primary | bollard        |               | x     |
		 | primary | some_tag       |               |       |

	Scenario: Access tag trumphs barriers
		Then routability should be
		 | highway | node/barrier   | node/access   | bothw |
		 | primary | bollard        | yes           | x     |
		 | primary | gate           | yes           | x     |
		 | primary | cattle_grid    | yes           | x     |
		 | primary | border_control | yes           | x     |
		 | primary | toll_booth     | yes           | x     |
		 | primary | sally_port     | yes           | x     |
		 | primary | bollard        | no            |       |
		 | primary | gate           | no            |       |
		 | primary | cattle_grid    | no            |       |
		 | primary | border_control | no            |       |
		 | primary | toll_booth     | no            |       |
		 | primary | sally_port     | no            |       |
		 | primary | sally_port     | some_tag      |       |
		 | primary | some_tag       | bicycle       | x     |
		 | primary | some_tag       | foot          |       |
		 | primary | some_tag       | motorcar      |       |
		 | primary | some_tag       | motor_vehicle |       |
		 | primary | some_tag       | vehicle       |       |
		 | primary | some_tag       | motorcar      |       |
		 | primary | some_tag       | permissive    |       |
		 | primary | some_tag       | designated    |       |