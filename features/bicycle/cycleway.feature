@routing @bicycle @cycleway
Feature: Bike - Cycle tracks/lanes
# Reference: http://wiki.openstreetmap.org/wiki/Key:cycleway

    Background:
        Given the profile "bicycle"

    Scenario: Bike - Cycle tracks/lanes should enable biking
        Then routability should be
            | highway     | foot | cycleway     | forw | backw |
            | motorway    | no   |              |      |       |
            | motorway    | no   | track        | x    |       |
            | motorway    | no   | lane         | x    |       |
            | motorway    | no   | shared       | x    |       |
            | motorway    | no   | share_busway | x    |       |
            | motorway    | no   | sharrow      | x    |       |
            | some_tag    | no   | track        | x    | x     |
            | some_tag    | no   | lane         | x    | x     |
            | some_tag    | no   | shared       | x    | x     |
            | some_tag    | no   | share_busway | x    | x     |
            | some_tag    | no   | sharrow      | x    | x     |
            | residential | no   | track        | x    | x     |
            | residential | no   | lane         | x    | x     |
            | residential | no   | shared       | x    | x     |
            | residential | no   | share_busway | x    | x     |
            | residential | no   | sharrow      | x    | x     |

    Scenario: Bike - Left/right side cycleways on implied bidirectionals
        Then routability should be
            | highway | foot | cycleway | cycleway:left | cycleway:right | forw | backw |
            | primary | no   |          |               |                | x    | x     |
            | primary | no   | track    |               |                | x    | x     |
            | primary | no   | opposite |               |                | x    | x     |
            | primary | no   |          | track         |                | x    | x     |
            | primary | no   |          | opposite      |                | x    | x     |
            | primary | no   |          |               | track          | x    | x     |
            | primary | no   |          |               | opposite       | x    | x     |
            | primary | no   |          | track         | track          | x    | x     |
            | primary | no   |          | opposite      | opposite       | x    | x     |
            | primary | no   |          | track         | opposite       | x    | x     |
            | primary | no   |          | opposite      | track          | x    | x     |

    Scenario: Bike - Left/right side cycleways on implied oneways
        Then routability should be
            | highway  | foot |cycleway | cycleway:left | cycleway:right | forw | backw |
            | primary  | no   |         |               |                | x    | x     |
            | motorway | no   |         |               |                |      |       |
            | motorway | no   |track    |               |                | x    |       |
            | motorway | no   |opposite |               |                |      | x     |
            | motorway | no   |         | track         |                |      | x     |
            | motorway | no   |         | opposite      |                |      | x     |
            | motorway | no   |         |               | track          | x    |       |
            | motorway | no   |         |               | opposite       | x    |       |
            | motorway | no   |         | track         | track          | x    | x     |
            | motorway | no   |         | opposite      | opposite       | x    | x     |
            | motorway | no   |         | track         | opposite       | x    | x     |
            | motorway | no   |         | opposite      | track          | x    | x     |

    Scenario: Bike - Invalid cycleway tags
        Then routability should be
            | highway  | foot | cycleway   | bothw |
            | primary  | no   |            | x     |
            | primary  | no   | yes        | x     |
            | primary  | no   | no         | x     |
            | primary  | no   | some_track | x     |
            | motorway | no   |            |       |
            | motorway | no   | yes        |       |
            | motorway | no   | no         |       |
            | motorway | no   | some_track |       |

    Scenario: Bike - Access tags should overwrite cycleway access
        Then routability should be
            | highway     | foot | cycleway | access | forw | backw |
            | motorway    | no   | track    | no     |      |       |
            | residential | no   | track    | no     |      |       |
            | footway     | no   | track    | no     |      |       |
            | cycleway    | no   | track    | no     |      |       |
            | motorway    | no   | lane     | yes    | x    |       |
            | residential | no   | lane     | yes    | x    | x     |
            | footway     | no   | lane     | yes    | x    | x     |
            | cycleway    | no   | lane     | yes    | x    | x     |
