@routing @oneways
Feature: Oneway streets
	Handle oneways streets, as defined at http://wiki.openstreetmap.org/wiki/OSM_tags_for_routing

	Scenario: Implied oneways
	 	Given the speedprofile "car"
	 	Then routability should be
		 | highway       | junction   | forw | backw |
		 | motorway      |            | x    | x     |
		 | motorway_link |            | x    |       |
		 | trunk         |            | x    | x     |
		 | trunk_link    |            | x    |       |
		 | primary       | roundabout | x    |       |

	Scenario: Overriding implied oneways
 		Given the defaults
	 	Then routability should be
		 | highway       | junction   | oneway | forw | backw |
		 | motorway_link |            | no     | x    | x     |
		 | trunk_link    |            | no     | x    | x     |
		 | primary       | roundabout | no     | x    | x     |
		 | motorway_link |            | -1     |      | x     |
		 | trunk_link    |            | -1     |      | x     |
		 | primary       | roundabout | -1     |      | x     |

	Scenario: Handle various oneway tag values
 		Given the defaults
	 	Then routability should be
		 | highway       | oneway   | forw | backw |
		 | primary       |          | x    | x     |
		 | primary       | nonsense | x    | x     |
		 | primary       | no       | x    | x     |
		 | primary       | false    | x    | x     |
		 | primary       | 0        | x    | x     |
		 | primary       | yes      | x    |       |
		 | primary       | true     | x    |       |
		 | primary       | 1        | x    |       |
		 | primary       | -1       |      | x     |

	Scenario: Disabling oneways in speedprofile
 		Given the speedprofile settings
		 | obeyOneways | no |
		Then routability should be
		 | highway       | junction   | oneway | forw | backw |
		 | primary       |            | yes    | x    | x     |
		 | primary       |            | true   | x    | x     |
		 | primary       |            | 1      | x    | x     |
		 | primary       |            | -1     | x    | x     |
		 | motorway_link |            |        | x    | x     |
		 | trunk_link    |            |        | x    | x     |
		 | primary       | roundabout |        | x    | x     |

	Scenario: Oneways and bicycles 
	 	Given the defaults
	 	Then routability should be
		 | highway       | junction   | oneway | oneway:bicycle | forw | backw |
		 | primary       |            |        | yes            | x    |       |
		 | primary       |            | yes    | yes            | x    |       |
		 | primary       |            | no     | yes            | x    |       |
		 | primary       |            | -1     | yes            | x    |       |
		 | motorway      |            |        | yes            | x    |       |
		 | motorway_link |            |        | yes            | x    |       |
		 | primary       | roundabout |        | yes            | x    |       |
		 | primary       |            |        | no             | x    | x     |
		 | primary       |            | yes    | no             | x    | x     |
		 | primary       |            | no     | no             | x    | x     |
		 | primary       |            | -1     | no             | x    | x     |
		 | motorway      |            |        | no             | x    | x     |
		 | motorway_link |            |        | no             | x    | x     |
		 | primary       | roundabout |        | no             | x    | x     |
		 | primary       |            |        | -1             |      | x     |
		 | primary       |            | yes    | -1             |      | x     |
		 | primary       |            | no     | -1             |      | x     |
		 | primary       |            | -1     | -1             |      | x     |
		 | motorway      |            |        | -1             |      | x     |
		 | motorway_link |            |        | -1             |      | x     |
		 | primary       | roundabout |        | -1             |      | x     |

	Scenario: Cars should not be affected by bicycle tags
	 	Given the speedprofile settings
		 | accessTag   | motorcar |
		
	 	Then routability should be
		 | highway | junction   | oneway | oneway:bicycle | forw | backw |
		 | primary |            | yes    | yes            | x    |       |
		 | primary |            | yes    | no             | x    |       |
		 | primary |            | yes    | -1             | x    |       |
		 | primary |            | no     | yes            | x    | x     |
		 | primary |            | no     | no             | x    | x     |
		 | primary |            | no     | -1             | x    | x     |
		 | primary |            | -1     | yes            |      | x     |
		 | primary |            | -1     | no             |      | x     |
		 | primary |            | -1     | -1             |      | x     |
		 | primary | roundabout |        | yes            | x    |       |
		 | primary | roundabout |        | no             | x    |       |
		 | primary | roundabout |        | -1             | x    |       |
