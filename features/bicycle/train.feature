@routing @bicycle @train @todo
Feature: Bike - Handle ferry routes
Bringing bikes on trains and subways

	Background:
		Given the speedprofile "bicycle"
	
	Scenario: Bike - Bringing bikes on trains
	 	Then routability should be
		 | highway | railway  | bicycle | bothw |
		 | primary |          |         | x     |
		 | (nil)   | train    |         |       |
		 | (nil)   | train    | no      |       |
		 | (nil)   | train    | yes     | x     |
		 | (nil)   | railway  |         |       |
		 | (nil)   | railway  | no      |       |
		 | (nil)   | railway  | yes     | x     |
		 | (nil)   | subway   |         |       |
		 | (nil)   | subway   | no      |       |
		 | (nil)   | subway   | yes     | x     |
		 | (nil)   | some_tag |         |       |
		 | (nil)   | some_tag | no      |       |
		 | (nil)   | some_tag | yes     | x     |
