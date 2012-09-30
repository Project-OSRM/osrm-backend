@routing @bicycle @way
Feature: Accessability of different way types

	Background:
		Given the speedprofile "bicycle"

	Scenario: Basic access for bicycles
	Bikes are allowed on footways etc because you can pull your bike at a lower speed.
	 	Given the speedprofile "bicycle"
	 	Then routability should be
		 | highway        | forw |
		 | motorway       |      |
		 | motorway_link  |      |
		 | trunk          |      |
		 | trunk_link     |      |
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
		 | cycleway       | x    |
		 | bridleway      |      |
