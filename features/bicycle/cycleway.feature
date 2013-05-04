@routing @bicycle @cycleway
Feature: Bike - Cycle tracks/lanes
Reference: http://wiki.openstreetmap.org/wiki/Key:cycleway

	Background:
		Given the profile "bicycle"
		
	Scenario: Bike - Cycle tracks/lanes should enable biking		
	 	Then routability should be
		 | highway     | cycleway     | bothw |
		 | motorway    |              |       |
		 | motorway    | track        | x     |
		 | motorway    | lane         | x     |
		 | motorway    | shared       | x     |
		 | motorway    | share_busway | x     |
		 | motorway    | sharrow      | x     |
		 | some_tag    | track        | x     |
		 | some_tag    | lane         | x     |
		 | some_tag    | shared       | x     |
		 | some_tag    | share_busway | x     |
		 | some_tag    | sharrow      | x     |
		 | residential | track        | x     |
		 | residential | lane         | x     |
		 | residential | shared       | x     |
		 | residential | share_busway | x     |
		 | residential | sharrow      | x     |

	Scenario: Bike - Left/right side cycleways on implied bidirectionals   
	 	Then routability should be
		 | highway | cycleway | cycleway:left | cycleway:right | forw | backw |
		 | primary |          |               |                | x    | x     |
		 | primary | track    |               |                | x    | x     |
		 | primary | opposite |               |                | x    | x     |
		 | primary |          | track         |                | x    | x     |
		 | primary |          | opposite      |                | x    | x     |
		 | primary |          |               | track          | x    | x     |
		 | primary |          |               | opposite       | x    | x     |
		 | primary |          | track         | track          | x    | x     |
		 | primary |          | opposite      | opposite       | x    | x     |
		 | primary |          | track         | opposite       | x    | x     |
		 | primary |          | opposite      | track          | x    | x     |

	Scenario: Bike - Left/right side cycleways on implied oneways   
	 	Then routability should be
		 | highway  | cycleway | cycleway:left | cycleway:right | forw | backw |
		 | primary  |          |               |                | x    | x     |
		 | motorway |          |               |                |      |       |
		 | motorway | track    |               |                | x    |       |
		 | motorway | opposite |               |                |      | x     |
		 | motorway |          | track         |                |      | x     |
		 | motorway |          | opposite      |                |      | x     |
		 | motorway |          |               | track          | x    |       |
		 | motorway |          |               | opposite       | x    |       |
		 | motorway |          | track         | track          | x    | x     |
		 | motorway |          | opposite      | opposite       | x    | x     |
		 | motorway |          | track         | opposite       | x    | x     |
		 | motorway |          | opposite      | track          | x    | x     |

	Scenario: Bike - Invalid cycleway tags		
	 	Then routability should be
		 | highway  | cycleway   | bothw |
		 | primary  |            | x     |
		 | primary  | yes        | x     |
		 | primary  | no         | x     |
		 | primary  | some_track | x     |
		 | motorway |            |       |
		 | motorway | yes        |       |
		 | motorway | no         |       |
		 | motorway | some_track |       |
			
	Scenario: Bike - Access tags should overwrite cycleway access		
	 	Then routability should be
		 | highway     | cycleway | access | bothw |
		 | motorway    | track    | no     |       |
		 | residential | track    | no     |       |
		 | footway     | track    | no     |       |
		 | cycleway    | track    | no     |       |
		 | motorway    | lane     | yes    | x     |
		 | residential | lane     | yes    | x     |
		 | footway     | lane     | yes    | x     |
		 | cycleway    | lane     | yes    | x     |