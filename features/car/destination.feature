@routing @car @destination
Feature: Car - Destination only, no passing through

    Background:
        Given the profile "car"

    Scenario: Car - Destination only street
        Given the node map
            """
            a       e
              b c d

            x       y
            """

        And the ways
            | nodes | access      |
            | ab    |             |
            | bcd   | destination |
            | de    |             |
            | axye  |             |

        When I route I should get
            | from | to | route      |
            | a    | b  | ab,ab      |
            | a    | c  | ab,bcd     |
            | a    | d  | ab,bcd,bcd |
            | a    | e  | axye,axye  |
            | e    | d  | de,de      |
            | e    | c  | de,bcd     |
            | e    | b  | de,bcd,bcd |
            | e    | a  | axye,axye  |

    Scenario: Car - Destination only street
        Given the node map
            """
            a       e
              b c d

            x       y
            """

        And the ways
            | nodes | access      |
            | ab    |             |
            | bc    | destination |
            | cd    | destination |
            | de    |             |
            | axye  |             |

        When I route I should get
            | from | to | route       |
            | a    | b  | ab,ab       |
            | a    | c  | ab,bc       |
            | a    | d  | ab,cd       |
            | a    | e  | axye,axye   |
            | e    | d  | de,de       |
            | e    | c  | de,cd       |
            | e    | b  | de,bc       |
            | e    | a  | axye,axye   |

    Scenario: Car - Routing inside a destination only area
        Given the node map
            """
            a   c   e
              b   d
            x       y
            """

        And the ways
            | nodes | access      |
            | ab    | destination |
            | bc    | destination |
            | cd    | destination |
            | de    | destination |
            | axye  |             |

        When I route I should get
            | from | to | route          |
            | a    | e  | ab,bc,cd,de,de |
            | e    | a  | de,cd,bc,ab,ab |
            | b    | d  | bc,cd,cd       |
            | d    | b  | cd,bc,bc       |
