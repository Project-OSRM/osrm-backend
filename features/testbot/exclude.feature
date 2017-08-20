@routing @testbot @exclude
Feature: Testbot - Exclude flags
    Background:
        Given the profile "testbot"
        Given the node map
            """
            a....b-----c-$-$-d
                 $     $     :
                 e.$.$.f.....g
            """

        And the ways
            | nodes | highway  | toll | #                                                                        |
            | ab    | primary  |      | always drivable                                                          |
            | bc    | motorway |      | not drivable for exclude=motorway and exclude=motorway,toll              |
            | be    | primary  | yes  | not drivable for exclude=toll and exclude=motorway,toll                  |
            | ef    | primary  | yes  | not drivable for exclude=toll and exclude=motorway,toll                  |
            | fc    | primary  | yes  | not drivable for exclude=toll and exclude=motorway,toll                  |
            | cd    | motorway | yes  | not drivable for exclude=motorway exclude=toll and exclude=motorway,toll |
            | fg    | primary  |      | always drivable                                                          |
            | gd    | primary  |      | always drivable                                                          |

    Scenario: Testbot - exclude nothing
        When I route I should get
            | from | to | route          |
            | a    | d  | ab,bc,cd,cd    |
            | a    | g  | ab,be,ef,fg,fg |
            | a    | c  | ab,bc,bc       |
            | a    | f  | ab,be,ef,ef    |

        When I match I should get
            | trace | matchings | duration |
            | ad    | ad        | 115      |

        When I request a travel time matrix I should get
            |   | a   | d   |
            | a | 0   | 115 |
            | d | 115 | 0   |

    Scenario: Testbot - exclude motorway
        Given the query options
            | exclude  | motorway        |

        When I route I should get
            | from | to | route             |
            | a    | d  | ab,be,ef,fg,gd,gd |
            | a    | g  | ab,be,ef,fg,fg    |
            | a    | c  | ab,be,ef,fc,fc    |
            | a    | f  | ab,be,ef,ef       |

        When I match I should get
            | trace | matchings | duration |
            | ad    | ad        | 125      |

        When I request a travel time matrix I should get
            |   | a   | d  |
            | a | 0   | 125 |
            | d | 125 | 0  |

    Scenario: Testbot - exclude toll
        Given the query options
            | exclude | toll |

        When I route I should get
            | from | to | route    |
            | a    | d  |          |
            | a    | g  |          |
            | a    | c  | ab,bc,bc |
            | a    | f  |          |
            | f    | d  | fg,gd,gd |

    Scenario: Testbot - exclude motorway and toll
        Given the query options
            | exclude | motorway,toll |

        When I route I should get
            | from | to | route    |
            | a    | d  |          |
            | a    | g  |          |
            | a    | c  |          |
            | a    | f  |          |
            | f    | d  | fg,gd,gd |

    Scenario: Testbot - exclude with unsupported exclude combination
        Given the query options
            | exclude | TwoWords2 |

        When I route I should get
            | from | to | status | message   |
            | a    | d  | 400    | Exclude flag combination is not supported. |

    Scenario: Testbot - exclude with invalid exclude class name
        Given the query options
            | exclude | foo |

        When I route I should get
            | from | to | status | message   |
            | a    | d  | 400    | Exclude flag combination is not supported. |

