@nearest
Feature: Locating Nearest node on a Way - pick closest way

    Background:
        Given the profile "testbot"

    Scenario: Nearest - two ways crossing
        Given the node map
            """
              0 c 1
            7   n   2
            a k x m b
            6   l   3
              5 d 4
            """

        And the ways
            | nodes |
            | axb   |
            | cxd   |

        When I request nearest I should get
            | in | out |
            | 0  | c   |
            | 1  | c   |
            | 2  | b   |
            | 3  | b   |
            | 4  | d   |
            | 5  | d   |
            | 6  | a   |
            | 7  | a   |
            | k  | k   |
            | l  | l   |
            | m  | m   |
            | n  | n   |

    Scenario: Nearest - inside a triangle
        Given the node map
            """
                      c

                  y       z
                    0   1
                  2   3   4
            a     x   u   w     b
            """

        And the ways
            | nodes |
            | ab    |
            | bc    |
            | ca    |

        When I request nearest I should get
            | in | out |
            | 0  | y   |
            | 1  | z   |
            | 2  | x   |
            | 3  | u   |
            | 4  | w   |

    Scenario: Nearest - inside a oneway triangle
        Given the node map
            """
                      c

                  y       z
                    0   1
                  2   3   4
            a     x   u   w     b
            """

        And the ways
            | nodes | oneway |
            | ab    | yes    |
            | bc    | yes    |
            | ca    | yes    |

        When I request nearest I should get
            | in | out |
            | 0  | y   |
            | 1  | z   |
            | 2  | x   |
            | 3  | u   |
            | 4  | w   |

    Scenario: Nearest - High lat/lon
        Given the node locations
            | node | lat     | lon  |
            | a    | -85     | -180 |
            | b    | -85     | -160 |
            | c    | -85     | -140 |
            | x    | -84.999 | -180 |
            | y    | -84.999 | -160 |
            | z    | -84.999 | -140 |

        And the ways
            | nodes |
            | abc   |

        When I request nearest I should get
            | in | out |
            | x  | a   |
            | y  | b   |
            | z  | c   |

    Scenario: Nearest - data version
        Given the node map
            """
                      c

                  y       z
                    0   1
                  2   3   4
            a     x   u   w     b
            """

        And the ways
            | nodes |
            | ab    |
            | bc    |
            | ca    |

        And the extract extra arguments "--data_version cucumber_data_version"

        When I request nearest I should get
            | in | out | data_version |
            | 0  | y   | cucumber_data_version |
            | 1  | z   | cucumber_data_version |
            | 2  | x   | cucumber_data_version |
            | 3  | u   | cucumber_data_version |
            | 4  | w   | cucumber_data_version |
