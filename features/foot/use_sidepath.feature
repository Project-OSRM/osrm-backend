@routing @foot @sidepath
Feature: Foot - foot=use_sidepath access
# Roads tagged foot=use_sidepath have a separately mapped compulsory parallel
# footway. Pedestrians must use that footway instead of the carriageway.
# Reference: https://wiki.openstreetmap.org/wiki/Tag:foot%3Duse_sidepath

    Background:
        Given the profile "foot"
        Given a grid size of 200 meters

    Scenario: Foot - routing prefers parallel footway over road tagged foot=use_sidepath
        Given the node map
            """
            a b
            c d
            """

        And the ways
            | nodes | highway | foot         | name      |
            | ab    | primary | use_sidepath | Main Road |
            | cd    | footway |              | Sidewalk  |
            | ac    | footway |              | Connector |
            | bd    | footway |              | Connector |

        When I route I should get
            | from | to | route               |
            | a    | b  | Connector,Connector |
