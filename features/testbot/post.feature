@post @testbot
Feature: POST request

    Background:
        Given the profile "testbot"
        And the HTTP method "POST"

    Scenario: Testbot - viaroute POST request
        Given the node locations
            | node | lat       | lon      |
            | a    | 55.68740  | 12.52430 |
            | b    | 55.68745  | 12.52409 |
            | c    | 55.68711  | 12.52383 |
            | x    | -55.68740 | 12.52430 |
            | y    | -55.68745 | 12.52409 |
            | z    | -55.68711 | 12.52383 |

        And the ways
            | nodes |
            | ab    |
            | bc    |
            | xy    |
            | yz    |
            
        When I route I should get
            | from | to | route | turns                  |
            | a    | c  | ab,bc | head,left,destination  |
            | c    | a  | bc,ab | head,right,destination |
            | x    | z  | xy,yz | head,right,destination |
            | z    | x  | yz,xy | head,left,destination  |

    Scenario: Testbot - match POST request
        Given the node map
            | a | b | c | d |
            | e | f | g | h |

        And the ways
            | nodes | oneway |
            | abcd  | yes    |
            | hgfe  | yes    |

        When I match I should get
            | trace | matchings |
            | dcba  | hgfe      |
            
    Scenario: Testbot - table POST request
        Given the node map
            | x | a | b | y |
            |   | d | e |   |

        And the ways
            | nodes | oneway |
            | abeda | yes    |
            | xa    |        |
            | by    |        |
            
        When I request a travel time matrix I should get
            |   | x   | y   | d   | e   |
            | x | 0   | 300 | 400 | 300 |
            | y | 500 | 0   | 300 | 200 |
            | d | 200 | 300 | 0   | 300 |
            | e | 300 | 400 | 100 | 0   |

    Scenario: Testbot - locate POST request
        Given the node locations
            | node | lat | lon  |
            | a    | -85 | -180 |
            | b    | 0   | 0    |
            | c    | 85  | 180  |
            | x    | -84 | -180 |
            | y    | 84  | 180  |

        And the ways
            | nodes |
            | abc   |

        When I request locate I should get
            | in | out |
            | x  | a   |
            | y  | c   |
            
    Scenario: Testbot - nearest POST request
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
