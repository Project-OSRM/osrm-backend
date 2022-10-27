@routing @testbot @exclude
Feature: Testbot - Exclude flags regression tests
    Background:
        Given the profile "testbot"

    Scenario: Testbot - Exclude toll regression 1
        Given the node map
            """
            a               g
            .               .
            b....d-$-$-e....f
            .               .
            c               h
            """

        And the ways
            | nodes | highway  | toll | #               |
            | ab    | primary  |      | always drivable |
            | cb    | primary  |      | always drivable |
            | bd    | primary  |      | always drivable |
            | de    | motorway | yes  | not drivable for exclude=toll |
            | ef    | primary  |      | always drivable |
            | fg    | primary  |      | always drivable |
            | fh    | primary  |      | always drivable |

        Given the query options
            | exclude | toll |

        When I route I should get
            | from | to | route         |
            | a    | h  |               |
            | a    | g  |               |
            | g    | a  |               |
            | d    | e  |               |

    Scenario: Testbot - Exclude toll regression 2
        Given the profile "testbot"

        Given the node map
            """
            a               g
            .               .
            b....d-$-$-e....f
            .               .
            c               h..i
            """

        And the ways
            | nodes | highway  | toll | #               |
            | ab    | primary  |      | always drivable |
            | cb    | primary  |      | always drivable |
            | bd    | primary  |      | always drivable |
            | de    | motorway | yes  | not drivable for exclude=toll |
            | ef    | primary  |      | always drivable |
            | fg    | primary  |      | always drivable |
            | fh    | primary  |      | always drivable |
            | hi    | primary  |      | always drivable |

        Given the query options
            | exclude | toll |

        When I route I should get
            | from | to | route         |
            | a    | h  |               |
            | a    | g  |               |
            | g    | a  |               |
            | d    | e  |               |
            | d    | i  |               |
