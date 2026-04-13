@routing @bicycle @sidepath
Feature: Bike - bicycle=use_sidepath access
# Roads tagged bicycle=use_sidepath have a separately mapped compulsory parallel
# cycleway. Cyclists must use that cycleway instead of the carriageway.
# Reference: https://wiki.openstreetmap.org/wiki/Tag:bicycle%3Duse_sidepath

    Background:
        Given the profile "bicycle"
        Given a grid size of 200 meters

    Scenario: Bike - routing prefers parallel cycleway over road tagged bicycle=use_sidepath
        Given the node map
            """
            a b
            c d
            """

        And the ways
            | nodes | highway  | bicycle      | name      |
            | ab    | primary  | use_sidepath | Main Road |
            | cd    | cycleway |              | Cycleway  |
            | ac    | cycleway |              | Connector |
            | bd    | cycleway |              | Connector |

        When I route I should get
            | from | to | route               |
            | a    | b  | Connector,Connector |

    Scenario: Bike - routing avoids road when cycleway is mapped as a separate way
        Given the node map
            """
            a b
            c d
            """

        And the ways
            | nodes | highway  | cycleway:right | name      |
            | ab    | primary  | separate       | Main Road |
            | cd    | cycleway |                | Cycleway  |
            | ac    | cycleway |                | Connector |
            | bd    | cycleway |                | Connector |

        When I route I should get
            | from | to | route               |
            | a    | b  | Connector,Connector |
