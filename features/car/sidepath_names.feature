@routing @car @sidepath
Feature: Car - Sidepath names should not affect car routing

    Background:
        Given the profile "car"
        Given a grid size of 200 meters

    Scenario: Car - Does not use sidepath name fallback on roads
        Given the node map
            """
            a b
            """

        And the ways
            | nodes | highway | name | street:name     |
            | ab    | primary |      | Should Not Show |

        When I route I should get
            | from | to | route |
            | a    | b  | ,     |

    Scenario: Car - Does not pick up is_sidepath:of:name on roads
        Given the node map
            """
            a b
            """

        And the ways
            | nodes | highway | name | is_sidepath:of:name |
            | ab    | primary |      | Should Not Show     |

        When I route I should get
            | from | to | route |
            | a    | b  | ,     |

    Scenario: Car - Sidepath markers do not affect car roads
        Given the node map
            """
            a b
            """

        And the ways
            | nodes | highway | footway  | name | is_sidepath:of:name |
            | ab    | primary | sidewalk |      | Should Not Show     |

        When I route I should get
            | from | to | route |
            | a    | b  | ,     |
