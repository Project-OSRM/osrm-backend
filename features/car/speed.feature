@routing @car @speed    
Feature: Car - speeds

    Background:
        Given the profile "car"
        And a grid size of 1000 meters
    
    Scenario: Car - speed of various way types
        Then routability should be
            | highway        | oneway | bothw   |
            | motorway       | no     | 90 km/h |
            | motorway_link  | no     | 75 km/h |
            | trunk          | no     | 85 km/h |
            | trunk_link     | no     | 70 km/h |
            | primary        | no     | 65 km/h |
            | primary_link   | no     | 60 km/h |
            | secondary      | no     | 55 km/h |
            | secondary_link | no     | 50 km/h |
            | tertiary       | no     | 40 km/h |
            | tertiary_link  | no     | 30 km/h |
            | unclassified   | no     | 25 km/h |
            | residential    | no     | 25 km/h |
            | living_street  | no     | 10 km/h |
            | service        | no     | 15 km/h |
