@routing @bicycle @way
Feature: Bike - Accessability of different way types

    Background:
        Given the profile "bicycle"

    # Bikes are allowed on footways etc because you can pull your bike at a lower speed.
    Scenario: Bike - Routability of way types

        Then routability should be
            | highway        | bothw |
            | (nil)          |       |
            | motorway       |       |
            | motorway_link  |       |
            | trunk          |       |
            | trunk_link     |       |
            | primary        | x     |
            | primary_link   | x     |
            | secondary      | x     |
            | secondary_link | x     |
            | tertiary       | x     |
            | tertiary_link  | x     |
            | residential    | x     |
            | service        | x     |
            | unclassified   | x     |
            | living_street  | x     |
            | road           | x     |
            | track          | x     |
            | path           | x     |
            | cycleway       | x     |
            | bridleway      |       |
            | footway        | x     |
            | pedestrian     | x     |
            | steps          | x     |
            | pier           | x     |

    Scenario: Bike - Routability of man_made structures
        Then routability should be
            | highway | man_made | bothw |
            | (nil)   | (nil)    |       |
            | (nil)   | pier     | x     |
