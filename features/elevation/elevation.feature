@routing @elevation
Feature: Route with elevation

    Background: Use some profile
        Given the profile "car"

    Scenario: Route and retrieve elevation - match on elevations
        Given the node map
            | a |   |   | d |
            |   | b |   |   |
            |   |   | c |   |

        And the ways
            | nodes |
            | ab    |
            | bc    |
            | cd    |

        And the nodes
            | node | ele        |
            | a    | 1732.18    |
            | b    | 98.45      |
            | c    | -32.10     |
            | d    | 2.4321     |


        When I route with elevation I should get
            | from | to | route | elevation              |
            | a    | c  | ab,bc | 1732.18 98.45 -32.10   |
            | b    | d  | bc,cd | -32.10 -32.10 2.43     |


    Scenario: Route and retrieve elevation - match on decoded geometry
        Given the node map
            | a |   |   | d |
            |   | b |   |   |
            |   |   | c |   |

        And the ways
            | nodes |
            | ab    |
            | bc    |
            | cd    |

        And the nodes
            | node | ele        |
            | a    | 1732.18    |
            | b    | 98.45      |
            | c    | -32.10     |
            | d    | 2.4321     |


        When I route with elevation I should get
            | from | to | route | geometry                                                                     |
            | a    | c  | ab,bc | 1.000000,1.000000,1732.18 0.999100,1.000899,98.45 0.998201,1.001798,-32.10   |
            | b    | d  | bc,cd | 0.999100,1.000899,-32.10 0.998201,1.001798,-32.10 1.000000,1.002697,2.43     |



@no_ele_data
    Scenario: Route with elevation without ele
        Given the node map
            | a |   |   | d |
            |   | b |   |   |
            |   |   | c |   |

        And the ways
            | nodes |
            | ab    |
            | bc    |
            | cd    |


        When I route with elevation I should get
            | from | to | route | geometry                                                                                    |
            | a    | c  | ab,bc | 1.000000,1.000000,21474836.47 0.999100,1.000899,21474836.47 0.998201,1.001798,21474836.47   |
            | b    | d  | bc,cd | 0.999100,1.000899,21474836.47 0.998201,1.001798,21474836.47 1.000000,1.002697,21474836.47   |

@no_ele_request
    Scenario: Route without elevation
        Given the node map
            | a |   |   | d |
            |   | b |   |   |
            |   |   | c |   |

        And the ways
            | nodes |
            | ab    |
            | bc    |
            | cd    |


        When I route I should get
            | from | to | route | geometry                    |
            | a    | c  | ab,bc | _c`\|@_c`\|@??fw@ew@dw@ew@  |
            | b    | d  | bc,cd | wj~{@e{a\|@??dw@ew@moBew@   |
# mind the '\' before the pipe

