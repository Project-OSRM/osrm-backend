@routing @bicycle @cycleway
Feature: Bike - Cycle tracks/lanes
# Reference: http://wiki.openstreetmap.org/wiki/Key:cycleway

    Background:
        Given the profile "bicycle"

    Scenario: Bike - Cycle tracks/lanes should enable biking
        Then routability should be
            | highway     | cycleway     | forw       | backw       |
            | motorway    | track        | cycling    |             |
            | motorway    | lane         | cycling    |             |
            | motorway    | shared       | cycling    |             |
            | motorway    | share_busway | cycling    |             |
            | motorway    | sharrow      | cycling    |             |
            | some_tag    | track        | cycling    | cycling     |
            | some_tag    | lane         | cycling    | cycling     |
            | some_tag    | shared       | cycling    | cycling     |
            | some_tag    | share_busway | cycling    | cycling     |
            | some_tag    | sharrow      | cycling    | cycling     |
            | residential | track        | cycling    | cycling     |
            | residential | lane         | cycling    | cycling     |
            | residential | shared       | cycling    | cycling     |
            | residential | share_busway | cycling    | cycling     |
            | residential | sharrow      | cycling    | cycling     |

    Scenario: Bike - Left/right side cycleways on implied bidirectionals
        Then routability should be
            | highway | cycleway | cycleway:left | cycleway:right | forw       | backw       |
            | primary |          |               |                | cycling    | cycling     |
            | primary | track    |               |                | cycling    | cycling     |
            | primary | opposite |               |                | cycling    | cycling     |
            | primary |          | track         |                | cycling    | cycling     |
            | primary |          | opposite      |                | cycling    | cycling     |
            | primary |          |               | track          | cycling    | cycling     |
            | primary |          |               | opposite       | cycling    | cycling     |
            | primary |          | track         | track          | cycling    | cycling     |
            | primary |          | opposite      | opposite       | cycling    | cycling     |
            | primary |          | track         | opposite       | cycling    | cycling     |
            | primary |          | opposite      | track          | cycling    | cycling     |

    Scenario: Bike - Left/right side cycleways on implied oneways
        Then routability should be
            | highway  | cycleway | cycleway:left | cycleway:right | forw    | backw   |
            | primary  |          |               |                | cycling | cycling |
            | motorway |          |               |                |         |         |
            | motorway | track    |               |                | cycling |         |
            | motorway | opposite |               |                |         | cycling |
            | motorway |          | track         |                |         | cycling |
            | motorway |          | opposite      |                |         | cycling |
            | motorway |          |               | track          | cycling |         |
            | motorway |          |               | opposite       | cycling |         |
            | motorway |          | track         | track          | cycling | cycling |
            | motorway |          | opposite      | opposite       | cycling | cycling |
            | motorway |          | track         | opposite       | cycling | cycling |
            | motorway |          | opposite      | track          | cycling | cycling |

    Scenario: Bike - Invalid cycleway tags
        Then routability should be
            | highway  | cycleway   | bothw   |
            | primary  |            | cycling |
            | primary  | yes        | cycling |
            | primary  | no         | cycling |
            | primary  | some_track | cycling |
            | motorway |            |         |
            | motorway | yes        |         |
            | motorway | no         |         |
            | motorway | some_track |         |

    Scenario: Bike - Access tags should overwrite cycleway access
        Then routability should be
            | highway     | cycleway | access | forw    | backw   |  
            | motorway    | track    | no     |         |         |  
            | residential | track    | no     |         |         |  
            | footway     | track    | no     |         |         |  
            | cycleway    | track    | no     |         |         |  
            | motorway    | lane     | yes    | cycling |         |  
            | residential | lane     | yes    | cycling | cycling |  
            | footway     | lane     | yes    | cycling | cycling |  
            | cycleway    | lane     | yes    | cycling | cycling |  

    Scenario: Bike - Cycleway on oneways, modes
        Then routability should be
            | highway     | cycleway | oneway | forw    | backw        |
            | motorway    | track    | yes    | cycling |              |
            | residential | track    | yes    | cycling | pushing bike |
            | cycleway    | track    | yes    | cycling |              |
            | footway     | track    | yes    | cycling | pushing bike |

    Scenario: Bike - Cycleway on oneways, speeds
        Then routability should be
            | highway     | cycleway | oneway | forw    | backw      |
            | motorway    | track    | yes    | 15 km/h |            |
            | residential | track    | yes    | 15 km/h | 5 km/h +-1 |
            | cycleway    | track    | yes    | 15 km/h |            |
            | footway     | track    | yes    | 15 km/h | 5 km/h +-1 |

    Scenario: Bike - Directional sideways and oneway
        Then routability should be
            | highway     | oneway | cycleway:right | cycleway:left | forw         | backw        |
            | primary     | yes    | track          |               | cycling      | pushing bike |
            | primary     | yes    |                | track         | cycling      | cycling      |
            | primary     | -1     | track          |               | cycling      | cycling      |
            | primary     | -1     |                | track         | pushing bike | cycling      |
