@routing @elevation @elevation_compiled
Feature: Route with elevation

    Background: Use some profile
        Given the profile "testbot"


    Scenario: Route and retrieve elevation - match on elevations - short
        Given the node map
            | a |   |
            |   | b |

        And the ways
            | nodes |
            | ab    |

        And the nodes
            | node | ele        |
            | a    | 1732.18    |
            | b    | 98.45      |

        And elevation is on

        And compression is off

        When I route I should get
            | from | to | route | elevation       |
            | a    | b  | ab    | 1732.18 98.45   |




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

        And elevation is on

        And compression is off

        When I route I should get
            | from | to | route    | elevation                  |
            | a    | c  | ab,bc    | 1732.18 98.45 -32.10       |
            | a    | d  | ab,bc,cd | 1732.18 98.45 -32.10 2.43  |
            | b    | c  | bc       | 98.45 -32.10               |
            | b    | d  | bc,cd    | 98.45 -32.10 2.43          |


    Scenario: Route and retrieve elevation - match on decoded geometry
        Given the node locations
            | node | lat | lon |
            | a    | 1.0 | 1.5 |
            | b    | 2.0 | 2.5 |
            | c    | 3.0 | 3.5 |
            | d    | 4.0 | 4.5 |

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


        And elevation is on

        And compression is off

        When I route I should get
            | from | to | route | geometry                                                                     |
            | a    | c  | ab,bc | 1.000000,1.500000,1732.18 2.000000,2.500000,98.45 3.000000,3.500000,-32.10   |
            | b    | d  | bc,cd | 2.000000,2.500000,98.45 3.000000,3.500000,-32.10 4.000000,4.500000,2.43      |



    Scenario: Route and retrieve elevation - match encoded geometry
       Given the node locations
            | node | lat | lon |
            | a    | 1.0 | 1.5 |
            | b    | 2.0 | 2.5 |
            | c    | 3.0 | 3.5 |
            | d    | 4.0 | 4.5 |

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


        And elevation is on

        When I route I should get
            | from | to | route | geometry                                          |
            | a    | c  | ab,bc | _c`\|@_upzAciqI_c`\|@_c`\|@xa~H_c`\|@_c`\|@\|nX   |
            | b    | d  | bc,cd | _gayB_yqwCifR_c`\|@_c`\|@\|nX_c`\|@_c`\|@yvE      |

# mind the '\' before the pipes
# polycodec.rb decode3 '_c`|@_upzAciqI_c`|@_c`|@xa~H_c`|@_c`|@|nX' [[1.0, 1.5, 0.173218], [2.0, 2.5, 0.00984500000000002], [3.0, 3.5, -0.0032099999999999802]]
# polycodec.rb decode3 '_gayB_yqwCifR_c`|@_c`|@|nX_c`|@_c`|@yvE' [[2.0, 2.5, 0.009845], [3.0, 3.5, -0.003210000000000001], [4.0, 4.5, 0.00024299999999999886]]




    Scenario: Route with elevation without ele tags in osm data
       Given the node locations
            | node | lat | lon |
            | a    | 1.0 | 1.5 |
            | b    | 2.0 | 2.5 |
            | c    | 3.0 | 3.5 |
            | d    | 4.0 | 4.5 |

        And the ways
            | nodes |
            | ab    |
            | bc    |
            | cd    |

        And elevation data is off

        And elevation is on

        And compression is off

        When I route I should get
            | from | to | route | geometry                                                                                    |
            | a    | c  | ab,bc | 1.000000,1.500000,21474836.47 2.000000,2.500000,21474836.47 3.000000,3.500000,21474836.47   |
            | b    | d  | bc,cd | 2.000000,2.500000,21474836.47 3.000000,3.500000,21474836.47 4.000000,4.500000,21474836.47   |

