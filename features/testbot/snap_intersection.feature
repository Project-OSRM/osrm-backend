Feature: Snapping at intersections

    Background:
        # Use turnbot so that we can validate when we are
        # snapping to one of many potential candidate ways
        Given the profile "turnbot"

    # https://github.com/Project-OSRM/osrm-backend/issues/4465
    Scenario: Snapping source to intersection with one-way roads
        Given the node map
            """
            a   e   c
              \ | /
                d

                1
            """

        And the ways
            | nodes | oneway |
            | da    | yes    |
            | dc    | yes    |
            | de    | yes    |


        When I route I should get
            | from | to | route  | time   |
            | 1    | e  | de,de  | 20s    |
            | 1    | a  | da,da  | 28.3s  |
            | 1    | c  | dc,dc  | 28.3s  |

        When I request a travel time matrix I should get
            |   | a        | c        | e   |
            | 1 | 28.3     | 28.3     | 20  |


    Scenario: Snapping destination to intersection with one-way roads
        Given the node map
            """
            a   e   c
              \ | /
                d

                1
            """

        And the ways
            | nodes | oneway |
            | da    | -1     |
            | dc    | -1     |
            | de    | -1     |


        When I route I should get
            | from | to | route  | time   |
            | e    | 1  | de,de  | 20s    |
            | a    | 1  | da,da  | 28.3s  |
            | c    | 1  | dc,dc  | 28.3s  |

        When I request a travel time matrix I should get
            |   | 1      |
            | a | 28.3   |
            | c | 28.3   |
            | e | 20     |


    Scenario: Snapping to intersection with bi-directional roads
        Given the node map
            """
            a   e
            | /
            d---c

            1
            """

        And the ways
            | nodes |
            | ad    |
            | ed    |
            | dc    |

        When I route I should get
            | from | to | route  | time   | weight |
            | 1    | c  | dc,dc  | 20s    |  20    |
            | 1    | a  | ad,ad  | 20s    |  20    |
            | 1    | e  | ed,ed  | 28.3s  |  28.3  |
            | c    | 1  | dc,dc  | 20s    |  20    |
            | a    | 1  | ad,ad  | 20s    |  20    |
            | e    | 1  | ed,ed  | 28.3s  |  28.3  |

        When I request a travel time matrix I should get
            |   | a    | c    | e     |
            | 1 | 20   | 20   | 28.3  |

        When I request a travel time matrix I should get
            |   | 1     |
            | a | 20    |
            | c | 20    |
            | e | 28.3  |


    Scenario: Snapping at compressible node
        Given the node map
            """
            a---b---c
            """

        And the ways
            | nodes |
            | abc   |

        When I route I should get
            | from | to | route    | time   | weight |
            | b    | c  | abc,abc  | 20s    |  20    |
            | b    | a  | abc,abc  | 20s    |  20    |
            | a    | b  | abc,abc  | 20s    |  20    |
            | c    | b  | abc,abc  | 20s    |  20    |


    Scenario: Snapping at compressible node with traffic lights
        Given the node map
            """
            a---b---c
            """

        And the ways
            | nodes |
            | abc   |

        # Turnbot will use the turn penalty instead of traffic penalty.
        # We do this to induce a penalty between two edges of the same
        # segment.
        And the nodes
            | node | highway         |
            | b    | traffic_signals |

        # Snaps to first edge in forward direction
        When I route I should get
            | from | to | route    | time   | weight |
            | b    | c  | abc,abc  | 40s    |  40    |
            | b    | a  | abc,abc  | 20s    |  20    |
            | a    | b  | abc,abc  | 20s    |  20    |
            | c    | b  | abc,abc  | 40s    |  40    |


    Scenario: Snapping at compressible node traffic lights, one-way
        Given the node map
            """
            a-->b-->c
            """

        And the ways
            | nodes | oneway |
            | abc   | yes    |

        # Turnbot will use the turn penalty instead of traffic penalty.
        # We do this to induce a penalty between two edges of the same
        # segment.
        And the nodes
            | node | highway         |
            | b    | traffic_signals |


        # Snaps to first edge in forward direction
        When I route I should get
            | from | to | route    | time   | weight |
            | b    | c  | abc,abc  | 40s    | 40     |
            | a    | b  | abc,abc  | 20s    | 20     |

        When I route I should get
            | from | to | code    |
            | b    | a  | NoRoute |
            | c    | b  | NoRoute |


    Scenario: Snapping at compressible node traffic lights, reverse one-way
        Given the node map
            """
            a<--b<--c
            """

        And the ways
            | nodes | oneway |
            | abc   | -1     |

        # Turnbot will use the turn penalty instead of traffic penalty.
        # We do this to induce a penalty between two edges of the same
        # segment.
        And the nodes
            | node | highway         |
            | b    | traffic_signals |


        # Snaps to first edge in forward direction - as this is one-way,
        # the forward direction has changed.
        When I route I should get
            | from | to | route    | time   | weight |
            | b    | a  | abc,abc  | 40s    | 40     |
            | c    | b  | abc,abc  | 20s    | 20     |

        When I route I should get
            | from | to | code    |
            | b    | c  | NoRoute |
            | a    | b  | NoRoute |


    Scenario: Snapping at traffic lights, reverse disabled
        Given the node map
            """
            a-->b-->c
            """

        And the ways
            | nodes |
            | abc   |

        And the contract extra arguments "--segment-speed-file {speeds_file}"
        And the customize extra arguments "--segment-speed-file {speeds_file}"
        And the speed file
            """
            2,1,0
            3,2,0
            """

        # Turnbot will use the turn penalty instead of traffic penalty.
        # We do this to induce a penalty between two edges of the same
        # segment.
        And the nodes
            | node | highway         |
            | b    | traffic_signals |

        # Snaps to first edge in forward direction.
        When I route I should get
            | from | to | route    | time   | weight |
            | b    | c  | abc,abc  | 40s    | 40     |
            | a    | b  | abc,abc  | 20s    | 20     |

        When I route I should get
            | from | to | code    |
            | b    | a  | NoRoute |
            | c    | b  | NoRoute |


    Scenario: Snapping at traffic lights, forward disabled
        Given the node map
            """
            a<--b<--c
            """

        And the ways
            | nodes |
            | abc   |

        And the contract extra arguments "--segment-speed-file {speeds_file}"
        And the customize extra arguments "--segment-speed-file {speeds_file}"
        And the speed file
            """
            1,2,0
            2,3,0
            """

        # Turnbot will use the turn penalty instead of traffic penalty.
        # We do this to induce a penalty between two edges of the same
        # segment.
        And the nodes
            | node | highway         |
            | b    | traffic_signals |

        # Forward direction is disabled, still snaps to first edge in forward direction
        When I route I should get
            | from | to | route    | time   | weight |
            | b    | a  | abc,abc  | 20s    | 20     |
            | c    | b  | abc,abc  | 40s    | 40     |

        When I route I should get
            | from | to | code    |
            | b    | c  | NoRoute |
            | a    | b  | NoRoute |


    Scenario: Snap to target node with next section of segment blocked
        Given the node map
            """
            a-->b---c---d<--e
            """

        And the ways
            | nodes |
            | abc   |
            | cde   |

        And the contract extra arguments "--segment-speed-file {speeds_file}"
        And the customize extra arguments "--segment-speed-file {speeds_file}"
        And the speed file
            """
            2,1,0
            4,5,0
            """

        When I route I should get
            | from | to | route       | time | weight |
            | a    | d  | abc,cde,cde | 60s  | 60     |
            | e    | b  | cde,abc,abc | 60s  | 60     |


        When I route I should get
            | from | to | code    |
            | a    | e  | NoRoute |
            | e    | a  | NoRoute |


    Scenario: Snapping to source node with previous section of segment blocked
        Given the node map
            """
            a<--b---c---d-->e
            """

        And the ways
            | nodes |
            | abc   |
            | cde   |

        And the contract extra arguments "--segment-speed-file {speeds_file}"
        And the customize extra arguments "--segment-speed-file {speeds_file}"
        And the speed file
            """
            1,2,0
            5,4,0
            """

        When I route I should get
            | from | to | code    |
            | a    | e  | NoRoute |
            | b    | e  | NoRoute |
            | e    | a  | NoRoute |
            | d    | a  | NoRoute |


    Scenario: Only snaps to one of many equidistant nearest locations
        Given the node map
            """
            b-------c
            |       |
            |       |
            a   1   d
            """

        And the ways
            | nodes |
            | ab    |
            | bc    |
            | cd    |

        When I route I should get
            | from | to | route    | time    | weight |
            | 1    | b  | ab,ab    | 30s     | 30     |
            | 1    | c  | ab,bc,bc | 80s +-1 | 80 +-1 |


    Scenario: Snaps to alternative big SCC candidate if nearest candidates are not strongly connected
        Given the node map
            """
                  1
              g---h---i
            a-----b-----c
                        |
            f-----e-----d
              j---k---l
                  2
            """

        Given the extract extra arguments "--small-component-size=4"

        And the ways
            | nodes |
            | abc   |
            | cd    |
            | fed   |
            | ghi   |
            | jkl   |

        # As forward direction is disabled...
        When I route I should get
            | from | to | route          | time     | weight   | locations |
            | 1    | 2  | abc,cd,fed,fed | 100s +-1 | 100 +-1  | b,c,d,e   |


    Scenario: Can use big or small SCC nearest candidates if at same location
        Given the node map
            """
                  1
            a-----b-----c
                  |     |
                  g     |
                        |
            f-----e-----d

            """

        Given the extract extra arguments "--small-component-size=4"

        And the ways
            | nodes | oneway | # comment  |
            | ab    | no     |            |
            | bc    | no     |            |
            | cd    | no     |            |
            | fed   | no     |            |
            | bg    | yes    | small SCC  |

        And the relations
            | type        | way:from | way:to | node:via | restriction   |
            | restriction | ab       | bg     | b        | no_right_turn |
            | restriction | bc       | bg     | b        | no_left_turn  |

        When I route I should get
            | from | to | route         | time     | weight  | locations |
            | 1    | g  | bg,bg         | 20s      | 20      | b,g       |
            | 1    | e  | bc,cd,fed,fed | 120s +-1 | 120 +-1 | b,c,d,e   |


    Scenario: Using small SCC candidates when at same location as big SCC alternatives is not supported
        Given the node map
            """
                  1
              g---h---i
            a-----b-----c
                  |     |
                  |     |
                  m     |
            f-----e-----d
              j---k---l
                  2

            """

        Given the extract extra arguments "--small-component-size=4"

        And the ways
            | nodes | oneway | # comment |
            | ab    | no     |           |
            | bc    | no     |           |
            | cd    | no     |           |
            | fed   | no     |           |
            | ghi   | no     |           |
            | jkl   | no     |           |
            | bm    | yes    | small SCC |

        And the relations
            | type        | way:from | way:to | node:via | restriction   |
            | restriction | ab       | bm     | b        | no_right_turn |
            | restriction | bc       | bm     | b        | no_left_turn  |

        When I route I should get
            | from | to | route         | time     | weight  | locations |
            | 1    | 2  | bc,cd,fed,fed | 120s +-1 | 120 +-1 | b,c,d,e   |
            | 1    | m  | bc,cd,fed,fed | 120s +-1 | 120 +-1 | b,c,d,e   |


    Scenario: Shortest via path with continuation, simple loop
        Given the node map
            """
            a---b
            """

        And the ways
            | nodes |
            | ab    |

        When I route I should get
            | waypoints | route       | time   | weight |
            | a,b,a     | ab,ab,ab,ab | 60s    | 60     |


    Scenario: Shortest via path with uturns, simple loop
        Given the node map
            """
            a---b
            """

        Given the query options
            | continue_straight | false |

        And the ways
            | nodes |
            | ab    |

        # Does not pay the cost of the turn
        When I route I should get
            | waypoints   | route                | time   | weight |
            | a,b,a       | ab,ab,ab,ab          | 40s    | 40     |


    Scenario: Shortest path with multiple endpoint snapping candidates
        Given the node map
            """
                b

                c

            a   d   f

                e
            """

        And the ways
            | nodes | oneway |
            | ab    | no     |
            | ac    | no     |
            | ad    | no     |
            | ae    | no     |
            | bf    | no     |
            | cf    | yes    |
            | df    | yes    |
            | ef    | no     |


        When I route I should get
            | from | to | route      | time   | weight |
            | a    | f  | ad,df,df   | 40s    |  40    |
            | f    | a  | ef,ae,ae   | 66.6s  |  66.6  |

        When I request a travel time matrix I should get
            |   | a     | f    |
            | a | 0     | 40   |
            | f | 66.6  | 0    |


    Scenario: Shortest via path with continuation, multiple waypoint snapping candidates
        Given the node map
            """
                b        g

                c        h

            a   d   f    i
                              k
                e        j
            """

        And the ways
            | nodes | oneway |
            | ab    | no     |
            | ac    | no     |
            | ad    | no     |
            | ae    | no     |
            | bf    | no     |
            | cf    | yes    |
            | df    | yes    |
            | ef    | no     |
            | fg    | no     |
            | fh    | -1     |
            | fi    | -1     |
            | fj    | no     |
            | gk    | no     |
            | hk    | no     |
            | ik    | no     |
            | kj    | no     |

        And the relations
            | type        | way:from | way:to | node:via | restriction     |
            | restriction | df       | fg     | f        | only_left_turn  |
            | restriction | fi       | bf     | f        | only_right_turn |

        # Longer routes can take different paths from sub-routes
        When I route I should get
            | waypoints | route                 | time   | weight |
            | a,f       | ad,df,df              | 40s    | 40     |
            | f,k       | fj,kj,kj              | 65.6s  | 65.6   |
            | a,f,k     | ac,cf,cf,fj,kj,kj     | 132.8s | 132.8  |
            | k,f       | ik,fi,fi              | 54.3s  | 54.3   |
            | f,a       | ef,ae,ae              | 66.6s  | 66.6   |
            | k,f,a     | kj,fj,fj,ef,ae,ae     | 141.4s | 141.4  |

        When I request a travel time matrix I should get
            |   |  a    |   f  |     k |
            | a |  0    |   40 | 132.8 |
            | f |  66.6 |   0  |  65.6 |
            | k | 141.4 | 54.3 |     0 |


    Scenario: Shortest via path with uturns, multiple waypoint snapping candidates
        Given the node map
            """
                b        g

                c        h

            a   d   f    i
                              k
                e        j
            """

        Given the query options
            | continue_straight | false |

        And the ways
            | nodes | oneway |
            | ab    | no     |
            | ac    | no     |
            | ad    | no     |
            | ae    | no     |
            | bf    | no     |
            | cf    | yes    |
            | df    | yes    |
            | ef    | no     |
            | fg    | no     |
            | fh    | -1     |
            | fi    | -1     |
            | fj    | no     |
            | gk    | no     |
            | hk    | no     |
            | ik    | no     |
            | kj    | no     |

        And the relations
            | type        | way:from | way:to | node:via | restriction     |
            | restriction | df       | fg     | f        | only_left_turn  |
            | restriction | fi       | bf     | f        | only_right_turn |

        # Longer routes use same path as sub-routes
        When I route I should get
            | waypoints | route                | time   | weight |
            | a,f       | ad,df,df             | 40s    | 40     |
            | f,k       | fj,kj,kj             | 65.6s  | 65.6   |
            | a,f,k     | ad,df,df,fj,kj,kj    | 105.6s | 105.6  |
            | k,f       | ik,fi,fi             | 54.3s  | 54.3   |
            | f,a       | ef,ae,ae             | 66.6s  | 66.6   |
            | k,f,a     | ik,fi,fi,ef,ae,ae    | 120.9s | 120.9  |
