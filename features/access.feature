@routing @access
Feature: Default speedprofiles
	Basic accessability of various way types depending on speedprofile.

	Scenario: Basic access for cars
	 	Given the speedprofile "car"
	 	Then routability should be
		 | highway       | forw |
		 | motorway      | x    |
		 | motorway_link | x    |
		 | trunk         | x    |
		 | trunk_link    | x    |
		 | primary       | x    |
		 | secondary     | x    |
		 | tertiary      | x    |
		 | residential   | x    |
		 | service       | x    |
		 | unclassified  | x    |
		 | living_street | x    |
		 | road		     | x    |
		 | track         |      |
		 | path          |      |
		 | footway       |      |
		 | pedestrian    |      |
		 | steps         |      |
		 | pier          |      |
		 | cycleway      |      |
		 | bridleway     |      |

	Scenario: Basic access for bicycles
	Bikes are allowed on footways etc because you can pull your bike at a lower speed.
	 	Given the speedprofile "bicycle"
	 	Then routability should be
		 | highway       | forw |
		 | motorway      |      |
		 | motorway_link |      |
		 | trunk         |      |
		 | trunk_link    |      |
		 | primary       | x    |
		 | secondary     | x    |
		 | tertiary      | x    |
		 | residential   | x    |
		 | service       | x    |
		 | unclassified  | x    |
		 | living_street | x    |
		 | road		     | x    |
		 | track         | x    |
		 | path          | x    |
		 | footway       | x    |
		 | pedestrian    | x    |
		 | steps         | x    |
		 | pier          | x    |
		 | cycleway      | x    |
		 | bridleway     |      |

	Scenario: Basic access for walking
	 	Given the speedprofile "foot"
	 	Then routability should be
		 | highway       | forw |
		 | motorway      |      |
		 | motorway_link |      |
		 | trunk         |      |
		 | trunk_link    | x    |
		 | primary       | x    |
		 | secondary     | x    |
		 | tertiary      | x    |
		 | residential   | x    |
		 | service       | x    |
		 | unclassified  | x    |
		 | living_street | x    |
		 | road		     | x    |
		 | track         | x    |
		 | path          | x    |
		 | footway       | x    |
		 | pedestrian    | x    |
		 | steps         | x    |
		 | pier          | x    |
		 | cycleway      | x    |
		 | bridleway     |      |

