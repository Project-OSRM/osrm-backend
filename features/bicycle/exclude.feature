@routing @bicycle @exclude
Feature: Bicycle - Exclude flags
    Background:
        Given the profile file "bicycle" initialized with
        """
        profile.excludable = Sequence { Set { 'ferry' } }
        """
        Given the node map
            """
            a....b~~~~~c...f
                 :     :
                 d.....e
            """

        And the ways
            | nodes | highway  | route | duration | #                                           |
            | ab    | service  |       |          | always drivable                             |
            | bc    |          | ferry | 00:00:01 | not drivable for exclude=ferry, but fast.   |
            | bd    | service  |       |          | always drivable                             |
            | de    | service  |       |          | always drivable                             |
            | ec    | service  |       |          | always drivable                             |
            | cf    | service  |       |          | always drivable                             |

    Scenario: Bicycle - exclude nothing
        When I route I should get
            | from | to | route          |
            | a    | f  | ab,bc,cf,cf    |

        When I match I should get
            | trace | matchings | duration |
            | abcf  | abcf      | 109      |

        When I request a travel time matrix I should get
            |   | a   | f   |
            | a | 0   | 109 |
            | f | 109 | 0   |

    Scenario: Bicycle - exclude ferry
        Given the query options
            | exclude  | ferry        |

        When I route I should get
            | from | to | route             |
            | a    | f  | ab,bd,de,ec,cf,cf |

        When I match I should get
            | trace | matchings | duration |
            | abcf  | abcf      | 301      |

        When I request a travel time matrix I should get
            |   | a          | f        |
            | a | 0          | 301 +- 1 |
            | f | 301.2 +- 1 | 0        |


