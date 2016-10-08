@routing @car @speed
Feature: Car - speeds

    Background:
        Given the profile "car"
        And a grid size of 1000 meters

    Scenario: Car - speed of various way types
        Then routability should be
            | highway        | oneway | bothw        |
            | motorway       | no     | 82 km/h      |
            | motorway_link  | no     | 47 km/h      |
            | trunk          | no     | 79 km/h +- 1 |
            | trunk_link     | no     | 43 km/h +- 1 |
            | primary        | no     | 63 km/h +- 1 |
            | primary_link   | no     | 34 km/h      |
            | secondary      | no     | 54 km/h +- 1 |
            | secondary_link | no     | 31 km/h      |
            | tertiary       | no     | 43 km/h      |
            | tertiary_link  | no     | 26 km/h      |
            | unclassified   | no     | 31 km/h      |
            | residential    | no     | 31 km/h      |
            | living_street  | no     | 18 km/h      |
            | service        | no     | 23 km/h      |

    # Alternating oneways have to take average waiting time into account.
    Scenario: Car - scaled speeds for oneway=alternating
        Then routability should be
            | highway        | oneway      | junction   | bothw        | #              |
            | tertiary       |             |            | 43 km/h      |                |
            | tertiary       | alternating |            | 20 km/h +- 5 |                |
            | motorway       |             |            | 82 km/h      | implied oneway |
            | motorway       | alternating |            | 30 km/h +- 5 | implied oneway |
            | motorway       | reversible  |            |              | unroutable     |
            | primary        |             | roundabout | 63 km/h      | implied oneway |
            | primary        | alternating | roundabout | 25 km/h +- 5 | implied oneway |
            | primary        | reversible  | roundabout |              | unroutable     |
