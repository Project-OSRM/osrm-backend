@routing @barrier
Feature: Barriers

	Scenario: Barriers and cars
	 	Given the speedprofile "car"
		Then routability should be
		 | highway | node/barrier   | node/access   | forw | backw |
		 | primary |                |               | x    | x     |
		 | primary | bollard        |               |      |       |
		 | primary | gate           |               |      |       |
		 | primary | cattle_grid    |               |      |       |
		 | primary | border_control |               |      |       |
		 | primary | toll_booth     |               |      |       |
		 | primary | sally_port     |               |      |       |
		 | primary | bollard        | yes           | x    | x     |
		 | primary | gate           | yes           | x    | x     |
		 | primary | cattle_grid    | yes           | x    | x     |
		 | primary | border_control | yes           | x    | x     |
		 | primary | toll_booth     | yes           | x    | x     |
		 | primary | sally_port     | yes           | x    | x     |
		 | primary | bollard        | motorcar      | x    | x     |
		 | primary | bollard        | motor_vehicle | x    | x     |
		 | primary | bollard        | vehicle       | x    | x     |
		 | primary | bollard        | motorcar      | x    | x     |
		 | primary | bollard        | permissive    | x    | x     |
		 | primary | bollard        | designated    | x    | x     |
	
	Scenario: Barriers and bicycles
	 	Given the speedprofile "bicycle"
		Then routability should be
		 | highway | node/barrier   | node/access   | forw | backw |
		 | primary |                |               | x    | x     |
		 | primary | bollard        |               | x    | x     |
		 | primary | gate           |               |      |       |
		 | primary | cattle_grid    |               |      |       |
		 | primary | border_control |               |      |       |
		 | primary | toll_booth     |               |      |       |
		 | primary | sally_port     |               |      |       |
		 | primary | bollard        | yes           | x    | x     |
		 | primary | gate           | yes           | x    | x     |
		 | primary | cattle_grid    | yes           | x    | x     |
		 | primary | border_control | yes           | x    | x     |
		 | primary | toll_booth     | yes           | x    | x     |
		 | primary | sally_port     | yes           | x    | x     |
		 | primary | bollard        | motorcar      |      |       |
		 | primary | bollard        | motor_vehicle |      |       |
		 | primary | bollard        | vehicle       | x    | x     |
		 | primary | bollard        | motorcar      |      |       |
		 | primary | bollard        | permissive    | x    | x     |
		 | primary | bollard        | designated    | x    | x     |
