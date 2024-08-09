@routing @countryfoot @countryway
Feature: Countryfoot - Accessability of different way types with countryspeeds

    Background:
	Given the profile file "countryfoot" initialized with
        """
        profile.uselocationtags.countryspeeds = true
        """

    Scenario: Countryfoot - Basic access
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
            | cycleway       |      |
            | bridleway      |      |

    Scenario: Countryfoot - Basic access
        Then routability should be
            | highway | leisure  | forw |
            | (nil)   | track    |   x  |

