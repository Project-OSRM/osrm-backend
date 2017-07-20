@routing @foot @way
Feature: Foot - Accessability of different way types

    Background:
        Given the profile "foot"

    Scenario: Foot - Routing on highway=*
        Then routability should be
            | highway        | bothw   |
            | motorway       |         |
            | motorway_link  |         |
            | trunk          |         |
            | trunk_link     |         |
            | primary        | walking |
            | primary_link   | walking |
            | secondary      | walking |
            | secondary_link | walking |
            | tertiary       | walking |
            | tertiary_link  | walking |
            | residential    | walking |
            | service        | walking |
            | unclassified   | walking |
            | living_street  | walking |
            | road           | walking |
            | track          | walking |
            | path           | walking |
            | footway        | walking |
            | pedestrian     | walking |
            | steps          | walking |
            | pier           | walking |
            | cycleway       |         |
            | bridleway      |         |


    Scenario: Foot - Basic access
        Then routability should be
            | highway | leisure  | forw |
            | (nil)   | track    |   x  | 