@routing @testbot @exclude
Feature: Testbot - Full exclusion capacity
    Background:
        Given the profile file "testbot" initialized with
        """
        profile.classes = { "motorway", "toll", "TooWords2", "c4", "c5", "c6", "c7" }
        profile.excludable = {
            {},
            { motorway = true },
            { toll = true },
            { motorway = true, toll = true },
            { c4 = true },
            { c5 = true, c6 = true },
            { motorway = true, toll = true, TooWords2 = true, c4 = true, c5 = true, c6 = true, c7 = true }
        }
        """
        And a grid size of 100 meters
        And the node map
            """
            a b c
            d e f
            """
        And the ways
            | nodes | highway  | toll |
            | ab    | motorway |      |
            | bc    | primary  | yes  |
            | ad    | primary  |      |
            | de    | primary  |      |
            | ef    | primary  |      |
            | fc    | primary  |      |

    Scenario: The default metric remains available at full exclusion capacity
        When I route I should get
            | from | to | route |
            | a    | c  | ab,bc |
        When I request a travel time matrix I should get
            |   | a  | c  |
            | a | 0  | 25 |
            | c | 25 | 0  |

    Scenario Outline: Every configured combination has its own metric
        Given the query options
            | exclude | <exclude> |
        When I route I should get
            | from | to | route   |
            | a    | c  | <route> |
        When I request a travel time matrix I should get
            |   | a          | c          |
            | a | 0          | <duration> |
            | c | <duration> | 0          |

        Examples:
            | exclude                             | route | duration |
            | motorway                            | ad,fc | 40       |
            | toll                                | ad,fc | 40       |
            | motorway,toll                       | ad,fc | 40       |
            | c4                                  | ab,bc | 25       |
            | c5,c6                               | ab,bc | 25       |
            | motorway,toll,TooWords2,c4,c5,c6,c7 | ad,fc | 40       |
