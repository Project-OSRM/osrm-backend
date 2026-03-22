@routing @foot @way
Feature: Foot - Accessability of different way types

    Background:
        Given the profile "foot"

    Scenario: Foot - Basic access
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
            | road           |      |
            | track          | x    |
            | path           | x    |
            | footway        | x    |
            | pedestrian     | x    |
            | steps          | x    |
            | pier           | x    |
            | cycleway       |      |
            | bridleway      |      |

    Scenario: Foot - Basic access
        Then routability should be
            | highway | leisure  | forw |
            | (nil)   | track    |   x  |

    Scenario: Foot - Proposed ways: unbuilt proposed highways are ignored, real highways with proposed upgrade tags are routed
        Then routability should be
            | highway  | foot  | proposed  | forw |
            | footway  |       |           | x    |
            | proposed |       |           |      |
            | proposed | yes   | yes       |      |
            | footway  |       | yes       | x    |
            | primary  |       | secondary | x    |

    Scenario: Foot - Motorroad
        Then routability should be
            | highway | motorroad  | forw |
            | primary | (nil)      | x    |
            | primary | yes        |      |
