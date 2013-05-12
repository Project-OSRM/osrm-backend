@routing @testbot @via
Feature: Via points
    
    Background:
        Given the profile "testbot"
    
        Scenario: Simple via point
            Given the node map
             | a | b | c |

            And the ways
             | nodes |
             | abc   |

            When I route I should get
             | waypoints | route |
             | a,b,c     | abc   |
             | c,b,a     | abc   |

    Scenario: Via point at a dead end
        Given the node map
         | a | b | c |
         |   | d |   |
    
        And the ways
         | nodes |
         | abc   |
         | bd    |
    
        When I route I should get
         | waypoints | route         |
         | a,d,c     | abc,bd,bd,abc |
         | c,d,a     | abc,bd,bd,abc |

     Scenario: Multiple via points
         Given the node map
          | a |   | c |   | e |   |
          |   | b |   | d |   | f |

         And the ways
          | nodes |
          | ace   |
          | bdf   |
          | ab    |
          | bc    |
          | cd    |
          | de    |
          | ef    |

         When I route I should get
          | waypoints   | route          |
          | a,b,c,d,e,f | ab,bc,cd,de,ef |
