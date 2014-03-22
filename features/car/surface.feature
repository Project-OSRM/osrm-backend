@routing @maxspeed @car
Feature: Car - Surface restrictions

    Background:
        Given the profile "car"
        Given a grid size of 1000 meters
        Given the node map
            | x | y |
            | z | w |
        Given the node locations
            | node | lat  | lon | \#                               |
            | a    | 0    | 0   |                                 |
            | b    | 0.05 | 0.5 | ~0.5% longer than straight to c |
            | c    | 0    | 1   |                                 |

    Scenario: Car - Do not route over extremely bad surfaces
        Then routability should be
            | tracktype | smoothness                  | bothw |
            | grade1    |                             | x     |
            | grade2    |                             | x     |
            | grade3    |                             | x     |
            | grade4    |                             | x     |
            | grade5    |                             | x     |
            | grade6    |                             |       |
            | grade7    |                             |       |
            | grade8    |                             |       |
            |           | excellent                   | x     |
            |           | thin_rollers                | x     |
            |           | good                        | x     |
            |           | thin_wheels                 | x     |
            |           | intermediate                | x     |
            |           | wheels                      | x     |
            |           | bad                         | x     |
            |           | robust_wheels               | x     |
            |           | very_bad                    | x     |
            |           | high_clearance              | x     |
            |           | horrible                    |       |
            |           | off_road_wheels             |       |
            |           | very_horrible               |       |
            |           | specialized_off_road_wheels |       |
            |           | impassable                  |       |
            | foobar    |                             |       |
            |           | foobar                      |       |

    Scenario: Car - Grade5 should be much worse than grade2
        Given the ways
            | nodes | highway  | tracktype |
            | xywz  | motorway | grade2    |
            | xz    | motorway | grade5    |

        When I route I should get
            | from | to | route |
            | x    | z  | xywz  |

    Scenario: Car - Smooth for high clearance should be much worse than smooth for robust wheels
        Given the ways
            | nodes | highway  | smoothness     |
            | xywz  | motorway | robust_wheels  |
            | xz    | motorway | high_clearance |

        When I route I should get
            | from | to | route |
            | x    | z  | xywz  |

    Scenario: Car - Smooth for robust wheels should be much worse than smooth for wheels
        Given the ways
            | nodes | highway  | smoothness    |
            | xywz  | motorway | wheels        |
            | xz    | motorway | robust_wheels |

        When I route I should get
            | from | to | route |
            | x    | z  | xywz  |

    Scenario: Car - Unspecified tracktype shouldn't be worse than grade1
        Given the ways
            | nodes | tracktype |
            | abc   | grade1    |
            | ac    |           |

        When I route I should get
            | from | to | route |
            | a    | c  | ac    |

    Scenario: Car - Grade1 shouldn't be worse than unspecified tracktype
        Given the ways
            | nodes | tracktype |
            | abc   |           |
            | ac    | grade1    |

        When I route I should get
            | from | to | route |
            | a    | c  | ac    |

    Scenario: Car - Grade2 should be worse than grade1
        Given the ways            
            | nodes | tracktype |
            | abc   | grade1    |
            | ac    | grade2    |

        When I route I should get
            | from | to | route |
            | a    | c  | abc   |

    Scenario: Car - Grade3 should be worse than grade2
        Given the ways            
            | nodes | tracktype |
            | abc   | grade2    |
            | ac    | grade3    |

        When I route I should get
            | from | to | route |
            | a    | c  | abc   |

    Scenario: Car - Grade4 should be worse than grade3
        Given the ways            
            | nodes | tracktype |
            | abc   | grade3    |
            | ac    | grade4    |

        When I route I should get
            | from | to | route |
            | a    | c  | abc   |

    Scenario: Car - Grade5 should be worse than grade4
        Given the ways            
            | nodes | tracktype |
            | abc   | grade4    |
            | ac    | grade5    |

        When I route I should get
            | from | to | route |
            | a    | c  | abc   |

    Scenario: Car - Grade2 should be worse than grade1 regardless of highway type
        Given the ways            
            | nodes | tracktype | highway     |
            | abc   | grade1    | residential |
            | ac    | grade2    | residential | 

        When I route I should get
            | from | to | route |
            | a    | c  | abc   |

    Scenario: Car - Grade2 should be worse than grade1 regardless of highway type
        Given the ways            
            | nodes | tracktype | highway  |
            | abc   | grade1    | motorway |
            | ac    | grade2    | motorway | 

        When I route I should get
            | from | to | route |
            | a    | c  | abc   |

    Scenario: Car - Grade3 should be worse than grade2 regardless of highway type
        Given the ways            
            | nodes | tracktype | highway     |
            | abc   | grade2    | residential |
            | ac    | grade3    | residential |

        When I route I should get
            | from | to | route |
            | a    | c  | abc   |

    Scenario: Car - Grade3 should be worse than grade2 regardless of highway type
        Given the ways            
            | nodes | tracktype | highway  |
            | abc   | grade2    | motorway |
            | ac    | grade3    | motorway |

        When I route I should get
            | from | to | route |
            | a    | c  | abc   |

    Scenario: Car - Grade4 should be worse than grade3 regardless of highway type
        Given the ways            
            | nodes | tracktype | highway     |
            | abc   | grade3    | residential |
            | ac    | grade4    | residential |

        When I route I should get
            | from | to | route |
            | a    | c  | abc   |

    Scenario: Car - Grade4 should be worse than grade3 regardless of highway type
        Given the ways            
            | nodes | tracktype | highway  |
            | abc   | grade3    | motorway |
            | ac    | grade4    | motorway |

        When I route I should get
            | from | to | route |
            | a    | c  | abc   |

    Scenario: Car - Grade5 should be worse than grade4 regardless of highway type
        Given the ways            
            | nodes | tracktype | highway     |
            | abc   | grade4    | residential |
            | ac    | grade5    | residential |

        When I route I should get
            | from | to | route |
            | a    | c  | abc   |

    Scenario: Car - Grade5 should be worse than grade4 regardless of highway type
        Given the ways            
            | nodes | tracktype | highway  |
            | abc   | grade4    | motorway |
            | ac    | grade5    | motorway |

        When I route I should get
            | from | to | route |
            | a    | c  | abc   |

    Scenario: Car - Unspecified smoothness shouldn't be worse than excellent smoothness
        Given the ways
            | nodes | smoothness |
            | abc   | excellent  |
            | ac    |            |

        When I route I should get
            | from | to | route |
            | a    | c  | ac    |

    Scenario: Car - Smooth for thin rollers shouldn't be worse than unspecified smoothness
        Given the ways
            | nodes | smoothness   |
            | abc   |              |
            | ac    | thin_rollers |

        When I route I should get
            | from | to | route |
            | a    | c  | ac    |

    Scenario: Car - Smooth for thin wheels should be worse than smooth for thin rollers
        Given the ways            
            | nodes | smoothness   |
            | abc   | thin_rollers |
            | ac    | thin_wheels  |

        When I route I should get
            | from | to | route |
            | a    | c  | abc   |

    Scenario: Car - Smooth for wheels should be worse than smoothness for thin wheels
        Given the ways            
            | nodes | smoothness  |
            | abc   | thin_wheels |
            | ac    | wheels      |

        When I route I should get
            | from | to | route |
            | a    | c  | abc   |

    Scenario: Car - Smooth for robust wheels should be worse than smooth for wheels
        Given the ways            
            | nodes | smoothness    |
            | abc   | wheels        |
            | ac    | robust_wheels |

        When I route I should get
            | from | to | route |
            | a    | c  | abc   |

    Scenario: Car - Smooth for high clearance should be worse than smooth for robust wheels
        Given the ways            
            | nodes | smoothness     |
            | abc   | robust_wheels  |
            | ac    | high_clearance |

        When I route I should get
            | from | to | route |
            | a    | c  | abc   |

    Scenario: Car - Smooth for thin wheels should be worse than smooth for thin rollers regardless of highway type
        Given the ways            
            | nodes | smoothness   | highway     |
            | abc   | thin_rollers | residential |
            | ac    | thin_wheels  | residential |

        When I route I should get
            | from | to | route |
            | a    | c  | abc   |

    Scenario: Car - Smooth for thin wheels should be worse than smooth for thin rollers regardless of highway type
        Given the ways            
            | nodes | smoothness   | highway  |
            | abc   | thin_rollers | motorway |
            | ac    | thin_wheels  | motorway |

        When I route I should get
            | from | to | route |
            | a    | c  | abc   |

    Scenario: Car - Smooth for wheels should be worse than smoothness for thin wheels regardless of highway type
        Given the ways            
            | nodes | smoothness  | highway     |
            | abc   | thin_wheels | residential |
            | ac    | wheels      | residential |

        When I route I should get
            | from | to | route |
            | a    | c  | abc   |

    Scenario: Car - Smooth for wheels should be worse than smoothness for thin wheels regardless of highway type
        Given the ways            
            | nodes | smoothness  | highway  |
            | abc   | thin_wheels | motorway |
            | ac    | wheels      | motorway |

        When I route I should get
            | from | to | route |
            | a    | c  | abc   |

    Scenario: Car - Smooth for robust wheels should be worse than smooth for wheels regardless of highway type
        Given the ways            
            | nodes | smoothness    | highway     |
            | abc   | wheels        | residential |
            | ac    | robust_wheels | residential |

        When I route I should get
            | from | to | route |
            | a    | c  | abc   |

    Scenario: Car - Smooth for robust wheels should be worse than smooth for wheels regardless of highway type
        Given the ways            
            | nodes | smoothness    | highway  |
            | abc   | wheels        | motorway |
            | ac    | robust_wheels | motorway |

        When I route I should get
            | from | to | route |
            | a    | c  | abc   |

    Scenario: Car - Smooth for high clearance should be worse than smooth for robust wheels regardless of highway type
        Given the ways            
            | nodes | smoothness     | highway     |
            | abc   | robust_wheels  | residential |
            | ac    | high_clearance | residential |

        When I route I should get
            | from | to | route |
            | a    | c  | abc   |

    Scenario: Car - Smooth for high clearance should be worse than smooth for robust wheels regardless of highway type
        Given the ways            
            | nodes | smoothness     | highway  |
            | abc   | robust_wheels  | motorway |
            | ac    | high_clearance | motorway |

        When I route I should get
            | from | to | route |
            | a    | c  | abc   |

    Scenario: Car - Smooth for thin rollers shouldn't be worse than excellent smoothness
        Given the ways
            | nodes | smoothness   |
            | abc   | excellent    |
            | ac    | thin_rollers |

        When I route I should get
            | from | to | route |
            | a    | c  | ac    |

    Scenario: Car - Excellent smoothness shouldn't be worse than smooth for thin rollers
        Given the ways
            | nodes | smoothness   |
            | abc   | thin_rollers |
            | ac    | excellent    |

        When I route I should get
            | from | to | route |
            | a    | c  | ac    |

    Scenario: Car - Smooth for thin wheels shouldn't be worse than good smoothness
        Given the ways
            | nodes | smoothness  |
            | abc   | good        |
            | ac    | thin_wheels |

        When I route I should get
            | from | to | route |
            | a    | c  | ac    |

    Scenario: Car - Good smoothness shouldn't be worse than smooth for thin wheels
        Given the ways
            | nodes | smoothness  |
            | abc   | thin_wheels |
            | ac    | good        |

        When I route I should get
            | from | to | route |
            | a    | c  | ac    |

    Scenario: Car - Smooth for wheels shouldn't be worse than intermediate smoothness
        Given the ways
            | nodes | smoothness   |
            | abc   | intermediate |
            | ac    | wheels       |

        When I route I should get
            | from | to | route |
            | a    | c  | ac    |

    Scenario: Car - Intermediate smoothness shouldn't be worse than smooth for wheels
        Given the ways
            | nodes | smoothness   |
            | abc   | wheels       |
            | ac    | intermediate |

        When I route I should get
            | from | to | route |
            | a    | c  | ac    |

    Scenario: Car - Smooth for robust wheels shouldn't be worse than bad smoothness
        Given the ways
            | nodes | smoothness    |
            | abc   | bad           |
            | ac    | robust_wheels |

        When I route I should get
            | from | to | route |
            | a    | c  | ac    |

    Scenario: Car - Bad smoothness shouldn't be worse than smooth for robust wheels
        Given the ways
            | nodes | smoothness    |
            | abc   | robust_wheels |
            | ac    | bad           |

        When I route I should get
            | from | to | route |
            | a    | c  | ac    |

    Scenario: Car - Smooth for high clearance shouldn't be worse than very bad smoothness
        Given the ways
            | nodes | smoothness     |
            | abc   | very_bad       |
            | ac    | high_clearance |

        When I route I should get
            | from | to | route |
            | a    | c  | ac    |

    Scenario: Car - Very bad smoothness shouldn't be worse than smooth for high clearance
        Given the ways
            | nodes | smoothness     |
            | abc   | high_clearance |
            | ac    | very_bad       |

        When I route I should get
            | from | to | route |
            | a    | c  | ac    |

    Scenario: Car - Smooth for off road wheels shouldn't be worse than horrible smoothness
        Given the ways
            | nodes | smoothness      |
            | abc   | horrible        |
            | ac    | off_road_wheels |

        When I route I should get
            | from | to | route |
            | a    | c  | ac    |

    Scenario: Car - Horrible smoothness shouldn't be worse than smooth for off road wheels
        Given the ways
            | nodes | smoothness      |
            | abc   | off_road_wheels |
            | ac    | horrible        |

        When I route I should get
            | from | to | route |
            | a    | c  | ac    |
