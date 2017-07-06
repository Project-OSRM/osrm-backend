@routing @bicycle @startpoint
Feature: Bike - Allowed start/end modes

    Background:
        Given the profile "bicycle"

    Scenario: Bike - Don't start/stop on ferries
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
            | 1    | 2  | ab,ab | cycling,cycling |
            | 2    | 1  | ab,ab | cycling,cycling |

    Scenario: Bike - Don't start/stop on trains
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
            | 1    | 2  | ab,ab | cycling,cycling |
            | 2    | 1  | ab,ab | cycling,cycling |

    Scenario: Bike - OK to start pushing bike
        Given the node map
            """
            a 1 b 2 c
            """

        And the ways
            | nodes | highway |
            | ab    | primary |
            | bc    | steps   |

        When I route I should get
            | from | to | route    | modes                             |
            | 1    | 2  | ab,bc,bc | cycling,pushing bike,pushing bike |
            | 2    | 1  | bc,ab,ab | pushing bike,cycling,cycling      |
