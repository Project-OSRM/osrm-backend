@routing @bicycle @way
Feature: Bike - Accessability of different way types

    Background:
        Given the profile "bicycle"

    Scenario: Bike - Routability of way types
    # Bikes are allowed on footways etc because you can pull your bike at a lower speed.
    # Pier is not allowed, since it's tagged using man_made=pier.

        Then routability should be
            | highway        | bothw        |
            | (nil)          |              |
            | motorway       |              |
            | motorway_link  |              |
            | trunk          |              |
            | trunk_link     |              |
            | primary        | cycling      |
            | primary_link   | cycling      |
            | secondary      | cycling      |
            | secondary_link | cycling      |
            | tertiary       | cycling      |
            | tertiary_link  | cycling      |
            | residential    | cycling      |
            | service        | cycling      |
            | unclassified   | cycling      |
            | living_street  | cycling      |
            | road           | cycling      |
            | track          | cycling      |
            | path           | cycling      |
            | footway        | pushing bike |
            | pedestrian     | pushing bike |
            | steps          | pushing bike |
            | cycleway       | cycling      |
            | bridleway      |              |
            | pier           | pushing bike |

    Scenario: Bike - Routability of man_made structures
        Then routability should be
            | highway | man_made | bothw |
            | (nil)   | (nil)    |       |
            | (nil)   | pier     | x     |
