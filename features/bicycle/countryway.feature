@routing @foot @countryway
Feature: bicycle - Accessability of different way types with countryspeeds

    Background:
	Given the profile file "bicycle" initialized with
        """
        profile.uselocationtags.countryspeeds = true
        """

    Scenario: Countrybicycle - Basic access
        Then routability should be
            | highway        | forw |
            | motorway       |      |
            | motorway_link  |      |
            | trunk          | x    |
            | trunk_link     | x    |
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
            | cycleway       | x    |
            | bridleway      |      |

    Scenario: Country bicycle - Routability of man_made structures
        Then routability should be
            | highway | man_made | bothw |
            | (nil)   | (nil)    |       |
            | (nil)   | pier     | x     |

