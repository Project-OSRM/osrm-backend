@trip @testbot
Feature: Basic trip planning

    Background:
        Given the profile "testbot"
        Given a grid size of 10 meters

    Scenario: Testbot - Trip planning with less than 10 nodes
        Given the node map
            | a | b |
            | d | c |

        And the ways
            | nodes |
            | ab    |
            | bc    |
            | cb    |
            | da    |

        When I plan a trip I should get
            | waypoints | trips |
            | a,b,c,d   | dcba  |

    Scenario: Testbot - Trip planning with more than 10 nodes
        Given the node map
            | a | b | c | d |
            | l |   |   | e |
            | k |   |   | f |
            | j | i | h | g |

        And the ways
            | nodes |
            | ab    |
            | bc    |
            | cb    |
            | de    |
            | ef    |
            | fg    |
            | gh    |
            | hi    |
            | ij    |
            | jk    |
            | kl    |
            | la    |


        When I plan a trip I should get
            | waypoints               | trips         |
            | a,b,c,d,e,f,g,h,i,j,k,l | cbalkjihgfedc |

    Scenario: Testbot - Trip planning with multiple scc
        Given the node map
            | a | b | c | d |
            | l |   |   | e |
            | k |   |   | f |
            | j | i | h | g |
            |   |   |   |   |
            | m | n |   |   |
            | p | o |   |   |

        And the ways
            | nodes |
            | ab    |
            | bc    |
            | cb    |
            | de    |
            | ef    |
            | fg    |
            | gh    |
            | hi    |
            | ij    |
            | jk    |
            | kl    |
            | la    |
            | mn    |
            | no    |
            | op    |
            | pm    |


        When I plan a trip I should get
            | waypoints                       | trips              |
            | a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p | cbalkjihgfedc,ponm |



