# The route results with #original are what the result should be if the maneuver tag is removed
@routing @guidance @maneuver
Feature: Maneuver tag support

    Background:
        Given the profile "car"
        Given a grid size of 5 meters

    Scenario: simple override #1
        Given the node map
            """
            a--b---c----d---e
                   |
                   g
                   |
            h------i--------j
            """
        And the ways
            | nodes | name     | oneway |
            | abc   | A Street | no     |
            | cde   | B Street | no     |
            | cgi   | C Street | no     |
            | hij   | J Street | no     |

        And the relations
            | type     | way:from | node:via | way:to | maneuver | direction   |
            | maneuver | abc      | c        | cgi    | turn     | sharp_right |
            | maneuver | hij      | i        | cde    | turn     | sharp_left  |
            | maneuver | abc      | c        | cde    | turn     | slight_left |
            | maneuver | cde      | c        | cgi    | turn     | straight    |
            | manoeuvre| cgi      | c        | abc    | turn     | right       |

        And the relations
            | type      | way:from | node:via | way:to | manoeuvre | maneuver | direction    |
            | maneuver  | cgi      | c        | cde    | fork      | turn     | slight_right |

        When I route I should get
            | waypoints | route                               | turns                                    |
        # Testing directly connected from/to
            | a,j       | A Street,C Street,J Street,J Street | depart,turn sharp right,turn left,arrive |
            | b,g       | A Street,C Street,C Street          | depart,turn sharp right,arrive           |
        # Testing re-awakening suppressed turns
            | a,e       | A Street,B Street,B Street          | depart,turn slight left,arrive           |
            | e,i       | B Street,C Street,C Street          | depart,turn straight,arrive              |
            | i,e       | C Street,B Street,B Street          | depart,fork slight right,arrive          |
            | i,a       | C Street,A Street,A Street          | depart,turn right,arrive                 |

    Scenario: single via-way
      Given the node map
          """"
            a--b---c----d---e
                   |
                   g
                   |
            h------i--------j
          """

        And the ways
            | nodes | name     | oneway |
            | abc   | A Street | no     |
            | cde   | B Street | no     |
            | cgi   | C Street | no     |
            | hi    | J Street | no     |
            | ij    | J Street | no     |

        And the relations
            | type     | way:from | way:via | way:to | node:via | maneuver | direction   |
            | maneuver | abc      | cgi     | ij     | c        | turn     | sharp_right |

        When I route I should get
            | waypoints | route                               | turns                                    |
            | a,j       | A Street,C Street,J Street,J Street | depart,turn sharp right,turn left,arrive |


    Scenario: multiple via-way
      Given the node map
          """"
            a--b---c----d---e
                   |
                   g-----k
                   |
            h------i--------j
          """

        And the ways
            | nodes | name     | oneway |
            | abc   | A Street | no     |
            | cde   | B Street | no     |
            | cg    | C Street | no     |
            | gi    | C Street | no     |
            | hi    | J Street | no     |
            | ij    | J Street | no     |
            | gk    | G Street | no     |

        And the relations
            | type     | way:from | way:via | way:via | way:to | node:via | maneuver | direction   |
            | maneuver | abc      | cg      | gi      | ij     | c        | turn     | sharp_right |

        When I route I should get
            | waypoints | route                               | turns                                    |
            | a,j       | A Street,C Street,J Street,J Street | depart,turn sharp right,end of road left,arrive |


    Scenario: Use maneuver tag to announce a particular turn type
        Given the node map
            """
            f
            *
            *
             *
              *
                *
                  *
                    *
                     *
                      *
             t. ..     *                h
                  .. ....m**           *
                        /    *       *
                       /       * * *
                      /
                     /
                    |
                    |
                     \
                      \
                       o
            """

        And the ways
            | nodes | name      | oneway | highway      |
            | fm    | CA-120    | no     | secondary    |
            | mh    | CA-120    | no     | secondary    |
            | mt    | Priest Rd | no     | unclassified |
            | mo    |           | no     | service      |

        And the relations
            | type     | way:from | node:via | way:to | maneuver | direction |
            | maneuver | mh       | m        | mt     | turn     | left      |

        When I route I should get
            | waypoints | route                        | turns                        |
            | h,t       | CA-120,Priest Rd,Priest Rd   | depart,turn left,arrive      |
  #original | h,t       | CA-120,Priest Rd,Priest Rd   | depart,turn straight,arrive  |

    Scenario: Use maneuver tag to announce lane guidance
        Given a grid size of 10 meters
        Given the node map
            """
               ad
              / \
             /   \
            /     \
            |     |
            |     |
            |     |
            b-----c------e
            |     |
            |     |
            |     |
            |     |
            r     w
            """

        And the ways
            | nodes | name      | oneway | highway   |
            | ab    | Marsh Rd  | yes    | secondary |
            | br    | Marsh Rd  | yes    | secondary |
            | cd    | Marsh Rd  | yes    | secondary |
            | cw    | Marsh Rd  | yes    | secondary |
            | bc    | service   | no     | service   |
            | ce    | service   | no     | service   |

        And the relations
            | type     | way:from | node:via | way:via | way:to | maneuver |
            | maneuver | ab       | c        | bc      | cd     | uturn    |
            | maneuver | ab       | b        | bc      | cd     | suppress |

        When I route I should get
            | waypoints | route                         | turns                    |
            | a,d       | Marsh Rd,Marsh Rd,Marsh Rd    | depart,turn uturn,arrive |
  #original | a,d       | Marsh Rd,service,Marsh Rd,Marsh Rd | depart,turn left,turn left,arrive |

    Scenario: Use maneuver tag to suppress a turn
        Given the node map
            """
              c
              |
              |
          v---y----------z
              |
          n---p----------k
              |\
              | \
              b  t
            """

        And the ways
            | nodes | name    | oneway | highway       |
            | zy    | NY Ave  | yes    | primary       |
            | yv    | NY Ave  | yes    | primary       |
            | np    | NY Ave  | yes    | primary       |
            | pk    | NY Ave  | yes    | primary       |
            | cp    | 4th St  | no     | tertiary      |
            | yp    |         | no     | motorway_link |
            | pb    | 4th St  | no     | primary       |
            | pt    | 395     | no     | primary       |

        And the relations
            | type     | way:from | node:via | way:via | way:to | maneuver | #                                                     |
            | maneuver | zy       | p        | yp      | pt     | suppress | original: depart,on ramp left,fork slight left,arrive |

        And the relations
            | type     | way:from | way:via | way:to | maneuver | #                                  |
            | maneuver | zy       | yp      | pb     | suppress | invalid relation: missing node:via |

        And the relations
            | type     | node:via | way:via | way:to | maneuver | #                                  |
            | maneuver | p        | yp      | pb     | suppress | invalid relation: missing way:from |

        And the relations
            | type     | way:from | node:via | way:via | maneuver | #                                |
            | maneuver | zy       | p        | yp      | suppress | invalid relation: missing way:to |

        And the relations
            | type     | way:from | node:via | way:via | way:to | maneuver | #                                   |
            | maneuver | zy       | y, p     | yp      | pb     | suppress | invalid relation: multiple node:via |

        When I route I should get
            | waypoints | route                 | turns                                        |
            | z,t       | NY Ave,395,395        | depart,on ramp left,arrive                   |
            | z,b       | NY Ave,,4th St,4th St | depart,on ramp left,fork slight right,arrive |
