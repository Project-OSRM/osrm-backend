@routing @bicycle @oneway
Feature: Bike - Oneway streets
Handle oneways streets, as defined at http://wiki.openstreetmap.org/wiki/OSM_tags_for_routing

	Background:
		Given the profile "bicycle"
	
	Scenario: Bike - Simple oneway
		Then routability should be
		 | highway | oneway | forw | backw |
		 | primary | yes    | x    |       |

	Scenario: Simple reverse oneway
		Then routability should be
		 | highway | oneway | forw | backw |
		 | primary | -1     |      | x     |

	Scenario: Bike - Around the Block
		Given the node map
		 | a | b |
		 | d | c |
	
		And the ways
		 | nodes | oneway |
		 | ab    | yes    |
		 | bc    |        |
		 | cd    |        |
		 | da    |        |
    
		When I route I should get
		 | from | to | route    |
		 | a    | b  | ab       |
		 | b    | a  | bc,cd,da |
	
	Scenario: Bike - Handle various oneway tag values
	 	Then routability should be
		 | oneway   | forw | backw |
		 |          | x    | x     |
		 | nonsense | x    | x     |
		 | no       | x    | x     |
		 | false    | x    | x     |
		 | 0        | x    | x     |
		 | yes      | x    |       |
		 | true     | x    |       |
		 | 1        | x    |       |
		 | -1       |      | x     |
	
	Scenario: Bike - Implied oneways
	 	Then routability should be
		 | highway       | bicycle | junction   | forw | backw |
		 |               |         |            | x    | x     |
		 |               |         | roundabout | x    |       |
		 | motorway      | yes     |            | x    |       |
		 | motorway_link | yes     |            | x    |       |
		 | motorway      | yes     | roundabout | x    |       |
		 | motorway_link | yes     | roundabout | x    |       |

	Scenario: Bike - Overriding implied oneways
	 	Then routability should be
		 | highway       | junction   | oneway | forw | backw |
		 | primary       | roundabout | no     | x    | x     |
		 | primary       | roundabout | yes    | x    |       |
		 | motorway_link |            | -1     |      |       |
		 | trunk_link    |            | -1     |      |       |
		 | primary       | roundabout | -1     |      | x     |
	
	Scenario: Bike - Oneway:bicycle should override normal oneways tags
	 	Then routability should be
		 | oneway:bicycle | oneway | junction   | forw | backw |
		 | yes            |        |            | x    |       |
		 | yes            | yes    |            | x    |       |
		 | yes            | no     |            | x    |       |
		 | yes            | -1     |            | x    |       |
		 | yes            |        | roundabout | x    |       |
		 | no             |        |            | x    | x     |
		 | no             | yes    |            | x    | x     |
		 | no             | no     |            | x    | x     |
		 | no             | -1     |            | x    | x     |
		 | no             |        | roundabout | x    | x     |
		 | -1             |        |            |      | x     |
		 | -1             | yes    |            |      | x     |
		 | -1             | no     |            |      | x     |
		 | -1             | -1     |            |      | x     |
		 | -1             |        | roundabout |      | x     |
	
	Scenario: Bike - Contra flow
	 	Then routability should be
		 | oneway | cycleway       | forw | backw |
		 | yes    | opposite       | x    | x     |
		 | yes    | opposite_track | x    | x     |
		 | yes    | opposite_lane  | x    | x     |
		 | -1     | opposite       | x    | x     |
		 | -1     | opposite_track | x    | x     |
		 | -1     | opposite_lane  | x    | x     |
		 | no     | opposite       | x    | x     |
		 | no     | opposite_track | x    | x     |
		 | no     | opposite_lane  | x    | x     |

	Scenario: Bike - Should not be affected by car tags
		Then routability should be
		 | junction   | oneway | oneway:car | forw | backw |
		 |            | yes    | yes        | x    |       |
		 |            | yes    | no         | x    |       |
		 |            | yes    | -1         | x    |       |
		 |            | no     | yes        | x    | x     |
		 |            | no     | no         | x    | x     |
		 |            | no     | -1         | x    | x     |
		 |            | -1     | yes        |      | x     |
		 |            | -1     | no         |      | x     |
		 |            | -1     | -1         |      | x     |
		 | roundabout |        | yes        | x    |       |
		 | roundabout |        | no         | x    |       |
		 | roundabout |        | -1         | x    |       |
