@routing @car @speed
Feature: Car - speeds

    Background:
        Given the profile "car"
        And a grid size of 1000 meters

    Scenario: Car - speed of various way types
        Then routability should be
            | highway        | oneway | bothw        |
            | motorway       | no     | 90 km/h +- 1 |
            | motorway_link  | no     | 45 km/h +- 1 |
            | trunk          | no     | 85 km/h +- 1 |
            | trunk_link     | no     | 40 km/h +- 1 |
            | primary        | no     | 63 km/h +- 1 |
            | primary_link   | no     | 30 km/h +- 1 |
            | secondary      | no     | 54 km/h +- 1 |
            | secondary_link | no     | 25 km/h +- 1 |
            | tertiary       | no     | 40 km/h +- 1 |
            | tertiary_link  | no     | 20 km/h +- 1 |
            | unclassified   | no     | 25 km/h +- 1 |
            | residential    | no     | 25 km/h +- 1 |
            | service        | no     | 15 km/h +- 1 |
            | living_street  | no     | 10 km/h +- 1 |
