@routing @car @startpoint
Feature: Car - Allowed start/end modes

    Background:
        Given the profile "car"

    Scenario: Car - Don't start/stop on ferries
        Given the node map
            """
            a 1 b 2 c
            """

        And the ways
            | nodes | highway | route | bicycle |
            | ab    | primary |       |         |
            | bc    |         | ferry | yes     |

        When I route I should get
            | from | to | route | modes           |
            | 1    | 2  | ab,ab | driving,driving |
            | 2    | 1  | ab,ab | driving,driving |

    Scenario: Car - Don't start/stop on trains
        Given the node map
            """
            a 1 b 2 c
            """

        And the ways
            | nodes | highway | railway | bicycle |
            | ab    | primary |         |         |
            | bc    |         | train   | yes     |

        When I route I should get
            | from | to | route | modes           |
            | 1    | 2  | ab,ab | driving,driving |
            | 2    | 1  | ab,ab | driving,driving |

    Scenario: Car - URL override of non-startpoints
        Given the node map
            """
            a 1 b   c 2 d
            """

        Given the query options
            | snapping  | any          |

        And the ways
            | nodes | highway | access  |
            | ab    | service | private |
            | bc    | primary |         |
            | cd    | service | private |

        When I request a travel time matrix I should get
            |   | 2    | c    |
            | 1 | 59.1 | 35.1 |
            | b | 35.1 | 11.1 |

        When I route I should get
            | from | to | route    |
            | 1    | 2  | ab,bc,cd |
            | 2    | 1  | cd,bc,ab |

    Scenario: Car - URL override of non-startpoints
        Given the node map
            """
            a 1 b   c 2 d
            """

        Given the query options
            | snapping  | any          |
            | bearings  | 90,180;      |

        And the ways
            | nodes | highway | access  |
            | ab    | service | private |
            | bc    | primary |         |
            | cd    | service | private |

        When I route I should get
            | from | to | route    |
            | 1    | 2  | ab,bc,cd |
            | 2    | 1  | cd,bc,ab |

    Scenario: Car - URL override of non-startpoints
        Given the node map
            """
            a 1 b   c 2 d
            """

        Given the query options
            | snapping  | any           |
            | radiuses  | 100;unlimited |

        And the ways
            | nodes | highway | access  |
            | ab    | service | private |
            | bc    | primary |         |
            | cd    | service | private |

        When I route I should get
            | from | to | route    |
            | 1    | 2  | ab,bc,cd |
            | 2    | 1  | cd,bc,ab |

    Scenario: Car - URL override of non-startpoints
        Given the node map
            """
            a 1 b   c 2 d
            """

        Given the query options
            | snapping  | any            |
            | bearings  | 90,180;0,180;; |

        And the ways
            | nodes | highway | access  |
            | ab    | service | private |
            | bc    | primary |         |
            | cd    | service | private |

        When I request a travel time matrix I should get
            |   | 2    | c    |
            | 1 | 59.1 | 35.1 |
            | b | 35.1 | 11.1 |