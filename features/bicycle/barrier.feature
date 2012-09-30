@routing @bicycle @barrier
Feature: Barriers

	Background:
		Given the speedprofile "bicycle"

	Scenario: Barriers and bicycles
	 	Given the speedprofile "bicycle"
		Then routability should be
		 | highway | node/barrier   | node/access   | bothw |
		 | primary |                |               | x     |
		 | primary | bollard        |               | x     |
		 | primary | gate           |               |       |
		 | primary | cattle_grid    |               |       |
		 | primary | border_control |               |       |
		 | primary | toll_booth     |               |       |
		 | primary | sally_port     |               |       |
		 | primary | bollard        | yes           | x     |
		 | primary | gate           | yes           | x     |
		 | primary | cattle_grid    | yes           | x     |
		 | primary | border_control | yes           | x     |
		 | primary | toll_booth     | yes           | x     |
		 | primary | sally_port     | yes           | x     |
		 | primary | bollard        | motorcar      |       |
		 | primary | bollard        | motor_vehicle |       |
		 | primary | bollard        | vehicle       | x     |
		 | primary | bollard        | motorcar      |       |
		 | primary | bollard        | permissive    | x     |
		 | primary | bollard        | designated    | x     |
