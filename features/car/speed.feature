@routing @car @speed
Feature: Car - speeds

    Background:
        Given the profile "car.lua"
        And a grid size of 1000 meters

    Scenario: Car - speed of various way types
        Then routability should be
            | highway        | oneway | bothw   |
            | motorway       | no     | 71 km/h |
            | motorway_link  | no     | 36 km/h |
            | trunk          | no     | 68 km/h |
            | trunk_link     | no     | 31 km/h |
            | primary        | no     | 52 km/h |
            | primary_link   | no     | 23 km/h |
            | secondary      | no     | 44 km/h |
            | secondary_link | no     | 19 km/h |
            | tertiary       | no     | 31 km/h |
            | tertiary_link  | no     | 16 km/h |
            | unclassified   | no     | 19 km/h |
            | residential    | no     | 19 km/h |
            | living_street  | no     | 8 km/h  |
            | service        | no     | 11 km/h |

    # Alternating oneways have to take average waiting time into account.
    Scenario: Car - scaled speeds for oneway=alternating
        Then routability should be
            | highway        | oneway      | junction   | forw          | backw        | #              |
            | tertiary       |             |            | 31 km/h       | 31 km/h      |                |
            | tertiary       | alternating |            | 12 km/h +- 1  | 12 km/h +- 1 |                |
            | motorway       |             |            | 71 km/h       |              | implied oneway |
            | motorway       | alternating |            | 28 km/h +- 1  |              | implied oneway |
            | motorway       | reversible  |            |               |              | unroutable     |
            | primary        |             | roundabout | 52 km/h       |              | implied oneway |
            | primary        | alternating | roundabout | 20 km/h +- 1  |              | implied oneway |
            | primary        | reversible  | roundabout |               |              | unroutable     |
