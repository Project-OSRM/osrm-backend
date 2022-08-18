@routing @car @speed
Feature: Car - speeds

    Background:
        Given the profile "car"
        And a grid size of 1000 meters

    # should more or less match default speeds in car profile, but may be different due to rounding errors
    Scenario: Car - speed of various way types
        Then routability should be
            | highway        | oneway | bothw   |
            | motorway       | no     | 90 km/h |
            | motorway_link  | no     | 45 km/h |
            | trunk          | no     | 84 km/h |
            | trunk_link     | no     | 40 km/h |
            | primary        | no     | 64 km/h |
            | primary_link   | no     | 30 km/h |
            | secondary      | no     | 54 km/h |
            | secondary_link | no     | 25 km/h |
            | tertiary       | no     | 40 km/h |
            | tertiary_link  | no     | 20 km/h |
            | unclassified   | no     | 25 km/h |
            | residential    | no     | 25 km/h |
            | living_street  | no     | 10 km/h  |
            | service        | no     | 15 km/h |

    # Alternating oneways scale rates but not speeds
    Scenario: Car - scaled speeds for oneway=alternating
        Then routability should be
            | highway        | oneway      | junction   | forw    | backw   | #              |
            | tertiary       |             |            | 40 km/h | 40 km/h |                |
            | tertiary       | alternating |            | 40 km/h | 40 km/h |                |
            | motorway       |             |            | 90 km/h |         | implied oneway |
            | motorway       | alternating |            | 90 km/h |         | implied oneway |
            | motorway       | reversible  |            |         |         | unroutable     |
            | primary        |             | roundabout | 64 km/h |         | implied oneway |
            | primary        | alternating | roundabout | 64 km/h |         | implied oneway |
            | primary        | reversible  | roundabout |         |         | unroutable     |

    Scenario: Car - Check roundoff errors
        Then routability should be

            | highway | maxspeed | forw    | backw    |
            | primary |          | 64 km/h | 64 km/h  |
            | primary | 60       | 48 km/h | 48 km/h  |
            | primary | 60       | 48 km/h | 48 km/h  |
            | primary | 60       | 48 km/h | 48 km/h  |

    Scenario: Car - Side road penalties
        Then routability should be

            | highway | side_road | forw    | backw    | forw_rate | backw_rate |
            | primary | yes       | 64 km/h | 64 km/h  | 14.5      | 14.5       |
