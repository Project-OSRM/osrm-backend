@routing @foot @way
Feature: Accessability of different way types

	Background:
		Given the speedprofile "foot"

	Scenario: Basic access for walking
	 	Then routability should be
		 | highway        | forw |
		 | motorway       |      |
		 | motorway_link  |      |
		 | trunk          |      |
		 | trunk_link     | x    |
		 | primary        | x    |
		 | primary_link   | x    |
		 | secondary      | x    |
		 | secondary_link | x    |
		 | tertiary       | x    |
		 | tertiary_link  | x    |
		 | residential    | x    |
		 | service        | x    |
		 | unclassified   | x    |
		 | living_street  | x    |
		 | road           | x    |
		 | track          | x    |
		 | path           | x    |
		 | footway        | x    |
		 | pedestrian     | x    |
		 | steps          | x    |
		 | pier           | x    |
		 | cycleway       |      |
		 | bridleway      |      |