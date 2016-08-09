@routing @testbot @sidebias
Feature: Testbot - side bias

    Scenario: Left hand bias
        Given the profile "lhs"
        And the node map
            | a |   | b |   | c |
            |   |   |   |   |   |
            |   |   | d |   |   |
        And the ways
            | nodes |
            | ab    |
            | bc    |
            | bd    |

        When I route I should get
            | from | to | route    | time       |
            | d    | a  | bd,ab,ab | 82s +-1    |
            | d    | c  | bd,bc,bc | 100s +-1   |

    Scenario: Right hand bias
        Given the profile "rhs"
        And the node map
            | a |   | b |   | c |
            |   |   |   |   |   |
            |   |   | d |   |   |
        And the ways
            | nodes |
            | ab    |
            | bc    |
            | bd    |

        When I route I should get
            | from | to | route    | time       |
            | d    | a  | bd,ab,ab | 100s +-1   |
            | d    | c  | bd,bc,bc | 82s +-1    |

    Scenario: Roundabout exit counting for left sided driving
        Given the profile "lhs"
        And a grid size of 10 meters
        And the node map
            |   |   | a |   |   |
            |   |   | b |   |   |
            | h | g |   | c | d |
            |   |   | e |   |   |
            |   |   | f |   |   |
       And the ways
            | nodes  | junction   |
            | ab     |            |
            | cd     |            |
            | ef     |            |
            | gh     |            |
            | bcegb  | roundabout |

       When I route I should get
           | waypoints | route    | turns                           |
           | a,d       | ab,cd,cd | depart,roundabout-exit-1,arrive |
           | a,f       | ab,ef,ef | depart,roundabout-exit-2,arrive |
           | a,h       | ab,gh,gh | depart,roundabout-exit-3,arrive |

     Scenario: Mixed Entry and Exit
        Given the profile "lhs"
        And a grid size of 10 meters
        And the node map
           |   | c |   | a |   |
           | j |   | b |   | f |
           |   | k |   | e |   |
           | l |   | h |   | d |
           |   | g |   | i |   |

        And the ways
           | nodes | junction   | oneway |
           | cba   |            | yes    |
           | fed   |            | yes    |
           | ihg   |            | yes    |
           | lkj   |            | yes    |
           | behkb | roundabout | yes    |

        When I route I should get
           | waypoints | route       | turns                           |
           | c,a       | cba,cba,cba | depart,roundabout-exit-1,arrive |
           | l,a       | lkj,cba,cba | depart,roundabout-exit-2,arrive |
           | i,a       | ihg,cba,cba | depart,roundabout-exit-3,arrive |
