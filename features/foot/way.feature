@routing @foot @way
Feature: Foot - Accessability of different way types

    Background:
        Given the profile "foot"

    Scenario: Foot - highway
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
            | road           | x    |
            | track          | x    |
            | path           | x    |
            | footway        | x    |
            | pedestrian     | x    |
            | steps          | x    |
            | pier           | x    |
            | cycleway       |      |
            | bridleway      |      |

    Scenario: Foot - leisure=*
        Then routability should be
            | highway | leisure | forw    |  
            | (nil)   |         |         |  
            | (nil)   | track   | walking |  

    Scenario: Foot - man_made=*
        Then routability should be
            | highway | man_made  | forw    |  
            | (nil)   |           |         |  
            | (nil)   | pier      | walking |  

    Scenario: Foot - railway=*
        Then routability should be
            | highway | railway  | forw    |  
            | (nil)   |          |         |  
            | (nil)   | platform | walking |  

    Scenario: Foot - public_transport=*
        Then routability should be
            | highway | public_transport  | forw    |  
            | (nil)   |                   |         |  
            | (nil)   | platform          | walking |  

    Scenario: Foot - amenity=*
        Then routability should be
            | highway | amenity | forw    |  
            | (nil)   |         |         |  
            | (nil)   | parking | walking |  
