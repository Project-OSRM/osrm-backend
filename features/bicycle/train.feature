@routing @bicycle @train
Feature: Bike - Handle ferry routes
# Bringing bikes on trains and subways
# We cannot currently use a 'routability' type test, since the bike
# profile does not allow starting/stopping on trains, and
# it's not possible to modify the bicycle profile table because it's
# defined as local.

    Background:
        Given the profile "bicycle"

    Scenario: Bike - Bringing bikes on trains
        Given the node map
            """
            a 1 b   c 2 d   e 3 f   g 4 h
            """

        And the ways
            | nodes | highway | railway | bicycle |
            | ab    | primary |         |         |
            | cd    | primary |         |         |
            | ef    | primary |         |         |
            | gh    | primary |         |         |
            | bc    |         | train   |         |
            | de    |         | train   | yes     |
            | fg    |         | train   | no      |

        When I route I should get
            | from | to | route       |
            | 1    | 2  |             |
            | 2    | 3  | cd,de,ef,ef |
            | 3    | 4  |             |

    Scenario: Bike - Bringing bikes on trains, invalid railway tag is accepted if access specified
        Given the node map
            """
            a 1 b   c 2 d   e 3 f   g 4 h
            """

        And the ways
            | nodes | highway | railway     | bicycle |
            | ab    | primary |             |         |
            | cd    | primary |             |         |
            | ef    | primary |             |         |
            | gh    | primary |             |         |
            | bc    |         | invalid_tag |         |
            | de    |         | invalid_tag | yes     |
            | fg    |         | invalid_tag | no      |

        When I route I should get
            | from | to | route |
            | 1    | 2  |       |
            | 2    | 3  | cd,de,ef|
            | 3    | 4  |       |

    @construction
    Scenario: Bike - Don't route on railways under construction
        Given the node map
            """
            a 1 b   c 2 d
            """

        And the ways
            | nodes | highway | railway      | bicycle |
            | ab    | primary |              |         |
            | cd    | primary |              |         |
            | bc    |         | construction | yes     |

        When I route I should get
            | from | to | route |
            | 1    | 2  |       |
