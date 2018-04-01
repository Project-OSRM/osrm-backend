@match @testbot
Feature: Basic Map Matching

    Background:
        Given the profile "testbot"
        Given a grid size of 10 meters
        Given the query options
            | geometries | geojson |

    Scenario: Testbot - Map matching with outlier that has no candidate
        Given a grid size of 100 meters
        Given the node map
            """
            a b c d

                1
            """

        And the ways
            | nodes | oneway |
            | abcd  | no     |

        When I match I should get
            | trace | timestamps | matchings |
            | ab1d  | 0 1 2 3    | ad        |

    Scenario: Testbot - Map matching with trace splitting
        Given the node map
            """
            a b c d
                e
            """

        And the ways
            | nodes | oneway |
            | abcd  | no     |

        When I match I should get
            | trace | timestamps | matchings |
            | abcd  | 0 1 62 63  | ab,cd     |

    Scenario: Testbot - Map matching with trace splitting suppression
        Given the query options
            | gaps | ignore |

        Given the node map
            """
            a b c d
                e
            """

        And the ways
            | nodes | oneway |
            | abcd  | no     |

        When I match I should get
            | trace | timestamps | matchings |
            | abcd  | 0 1 62 63  | abcd      |

    Scenario: Testbot - Map matching with trace tidying. Clean case.
        Given a grid size of 100 meters

        Given the query options
            | tidy | true |

        Given the node map
            """
            a b c d
                e
            """

        And the ways
            | nodes | oneway |
            | abcd  | no     |

        When I match I should get
            | trace | timestamps | matchings |
            | abcd | 0 10 20 30  | abcd      |

    Scenario: Testbot - Map matching with trace tidying. Dirty case by ts.
        Given a grid size of 100 meters

        Given the query options
            | tidy | true |

        Given the node map
            """
            a b c d
                e
            """

        And the ways
            | nodes | oneway |
            | abcd  | no     |

        When I match I should get
            | trace | timestamps    | matchings |
            | abacd | 0 10 12 20 30 | abcd      |

    Scenario: Testbot - Map matching with trace tidying. Dirty case by dist.
        Given a grid size of 8 meters

        Given the query options
            | tidy | true |

        Given the node map
            """
            a q b c d
                e
            """

        And the ways
            | nodes | oneway |
            | aqbcd | no     |

        When I match I should get
            | trace | matchings |
            | abcbd | abbd      |

    Scenario: Testbot - Map matching with core factor
        Given the contract extra arguments "--core 0.8"
        Given the node map
            """
            a b c d
                e
            """

        And the ways
            | nodes | oneway |
            | abcd  | no     |

        When I match I should get
            | trace | timestamps | matchings |
            | abcd  | 0 1 2 3    | abcd      |

    Scenario: Testbot - Map matching with small distortion
        Given the node map
            """
            a b c d e
              f



              h     k
            """

        # The second way does not need to be a oneway
        # but the grid spacing triggers the uturn
        # detection on f
        And the ways
            | nodes | oneway |
            | abcde | no     |
            | bfhke | yes    |

        When I match I should get
            | trace  | matchings |
            | afcde  | abcde     |

    Scenario: Testbot - Map matching with oneways
        Given a grid size of 10 meters
        Given the node map
            """
            a b c d
            e f g h
            """

        And the ways
            | nodes | oneway |
            | abcd  | yes    |
            | hgfe  | yes    |

        When I match I should get
            | trace | matchings |
            | dcba  | hgfe      |

    Scenario: Testbot - Matching with oneway streets
        Given a grid size of 10 meters
        Given the node map
            """
            a b c d
            e f g h
            """

        And the ways
            | nodes | oneway |
            | ab    | yes    |
            | bc    | yes    |
            | cd    | yes    |
            | hg    | yes    |
            | gf    | yes    |
            | fe    | yes    |

        When I match I should get
            | trace | matchings   |
            | dcba  | hgfe        |
            | efgh  | abcd        |

    Scenario: Testbot - request duration annotations
        Given the query options
            | annotations | duration |

        Given the node map
            """
            a b c d e   g h
                i
            """

        And the ways
            | nodes    | oneway |
            | abcdegh  | no     |
            | ci       | no     |

        And the speed file
        """
        1,2,36,10
        """

        And the contract extra arguments "--segment-speed-file {speeds_file}"
        And the customize extra arguments "--segment-speed-file {speeds_file}"

        When I match I should get
            | trace | matchings | a:duration       |
            | ach   | ach       | 1:1,0:1:1:2:1    |

    Scenario: Testbot - Duration details
        Given the query options
            | annotations | duration,nodes |

        Given the node map
            """
            a b c d e   g h
                i
            """

        And the ways
            | nodes    | oneway |
            | abcdegh  | no     |
            | ci       | no     |

        And the speed file
        """
        1,2,36,10
        """

        And the contract extra arguments "--segment-speed-file {speeds_file}"
        And the customize extra arguments "--segment-speed-file {speeds_file}"

        When I match I should get
            | trace | matchings | a:duration      |
            | abeh  | abeh      | 1:0,1:1:1,0:2:1 |
            | abci  | abci      | 1:0,1,0:1       |

        # The following is the same as the above, but separated for readability (line length)
        When I match I should get
            | trace | matchings | a:nodes               |
            | abeh  | abeh      | 1:2:3,2:3:4:5,4:5:6:7 |
            | abci  | abci      | 1:2:3,2:3,2:3:8       |

    Scenario: Testbot - Regression test for #3037
        Given the query options
            | overview   | simplified |
            | geometries | geojson  |

        Given the node map
            """
            a--->---b--->---c
            |       |       |
            |       ^       |
            |       |       |
            e--->---f--->---g
            """

        And the ways
            | nodes | oneway |
            | abc   | yes    |
            | efg   | yes    |
            | ae    | yes    |
            | cg    | yes    |
            | fb    | yes    |

        When I match I should get
            | trace | matchings | geometry                                      |
            | efbc  | efbc      | 1,0.99964,1.00036,0.99964,1.00036,1,1.000719,1 |

    Scenario: Testbot - Geometry details using geojson
        Given the query options
            | overview   | full     |
            | geometries | geojson  |

        Given the node map
            """
            a b c
              d
            """

        And the ways
            | nodes | oneway |
            | abc   | no     |
            | bd    | no     |

        When I match I should get
            | trace | matchings | geometry                                |
            | abd   | abd       | 1,1,1.00009,1,1.00009,1,1.00009,0.99991 |

    Scenario: Testbot - Geometry details using polyline
        Given the query options
            | overview   | full      |
            | geometries | polyline  |

        Given the node map
            """
            a b c
              d
            """

        And the ways
            | nodes | oneway |
            | abc   | no     |
            | bd    | no     |

        When I match I should get
            | trace | matchings | geometry                                |
            | abd   | abd       | 1,1,1,1.00009,1,1.00009,0.99991,1.00009 |

    Scenario: Testbot - Geometry details using polyline6
        Given the query options
            | overview   | full       |
            | geometries | polyline6  |

        Given the node map
            """
            a b c
              d
            """

        And the ways
            | nodes | oneway |
            | abc   | no     |
            | bd    | no     |

        When I match I should get
            | trace | matchings | geometry                                |
            | abd   | abd       | 1,1,1,1.00009,1,1.00009,0.99991,1.00009 |

    Scenario: Testbot - Matching alternatives count test
        Given the node map
            """
            a b c d e f
                  g h i
            """

        And the ways
            | nodes  | oneway |
            | abcdef | yes    |
            | dghi   | yes    |

        When I match I should get
            | trace  | matchings | alternatives         |
            | abcdef | abcde     | 0,0,0,0,1,1          |

    Scenario: Testbot - Speed greater than speed threshold
        Given a grid size of 100 meters
        Given the query options
            | geometries | geojson  |

        Given the node map
            """
            a b ---- x
                     |
                     |
                     y --- c d
            """

        And the ways
            | nodes   | oneway |
            | abxycd  | no     |

        When I match I should get
            | trace | timestamps | matchings |
            | abcd  | 0 1 2 3    | ab,cd     |

    Scenario: Testbot - Speed less than speed threshold
        Given a grid size of 100 meters
        Given the query options
            | geometries | geojson  |

        Given the node map
            """
            a b c d
            """

        And the ways
            | nodes | oneway |
            | abcd  | no     |

        When I match I should get
            | trace | timestamps | matchings |
            | abcd  | 0 1 2 3    | abcd      |

    Scenario: Testbot - Huge gap in the coordinates
        Given a grid size of 100 meters
        Given the query options
            | geometries | geojson  |
            | gaps | ignore |

        Given the node map
            """
            a b c d ---- x
                         |
                         |
                         y ---- z ---- efjk
            """

        And the ways
            | nodes   | oneway |
            | abcdxyzefjk  | no     |

        When I match I should get
            | trace     | timestamps           | matchings  |
            | abcdefjk  | 0 1 2 3 50 51 52 53  | abcdefjk   |

    # Regression test 1 for issue 3176
    Scenario: Testbot - multiple segments: properly expose OSM IDs
        Given the query options
            | annotations | true    |

        Given the node map
            """
            a-1-b--c--d--e--f-2-g
            """

        And the nodes
            | node | id |
            | a    | 1  |
            | b    | 2  |
            | c    | 3  |
            | d    | 4  |
            | e    | 5  |
            | f    | 6  |
            | g    | 7  |

        And the ways
            | nodes | oneway |
            | ab    | no     |
            | bc    | no     |
            | cd    | no     |
            | de    | no     |
            | ef    | no     |
            | fg    | no     |

        When I match I should get
            | trace | a:nodes       |
            | 12    | 1:2:3:4:5:6:7 |
            | 21    | 7:6:5:4:3:2:1 |

    # Regression test 2 for issue 3176
    Scenario: Testbot - same edge: properly expose OSM IDs
        Given the query options
            | annotations | true    |

        Given the node map
            """
            a-1-b--c--d--e-2-f
            """

        And the nodes
            | node | id |
            | a    | 1  |
            | b    | 2  |
            | c    | 3  |
            | d    | 4  |
            | e    | 5  |
            | f    | 6  |

        And the ways
            | nodes   | oneway |
            | abcdef  | no     |

        When I match I should get
            | trace | a:nodes     |
            | 12    | 1:2:3:4:5:6 |
            | 21    | 6:5:4:3:2:1 |


    Scenario: Matching with waypoints param for start/end
        Given the node map
            """
            a-----b---c
                  |
                  |
                  d
                  |
                  |
                  e
            """
        And the ways
            | nodes | oneway |
            | abc   | no     |
            | bde   | no     |

        Given the query options
            | waypoints | 0;3   |

        When I match I should get
            | trace | code    | matchings | waypoints |
            | abde  | Ok      | abde      | ae        |

    Scenario: Matching with waypoints param that were tidied away
        Given the node map
            """
            a - b - c - e
                    |
                    f
                    |
                    g
            """
        And the ways
            | nodes | oneway |
            | abce  | no     |
            | cfg   | no     |

        Given the query options
            | tidy      | true    |
            | waypoints | 0;2;5   |

        When I match I should get
            | trace  | code    | matchings | waypoints |
            | abccfg | Ok      | abcfg     | acg       |

    Scenario: Testbot - Map matching refuses to use waypoints with trace splitting
        Given the node map
            """
            a b c d
                e
            """
        Given the query options
            | waypoints | 0;3   |

        And the ways
            | nodes | oneway |
            | abcd  | no     |

        When I match I should get
            | trace | timestamps | code     |
            | abcd  | 0 1 62 63  | NoMatch  |

    Scenario: Testbot - Map matching invalid waypoints
        Given the node map
            """
            a b c d
                e
            """
        Given the query options
            | waypoints | 0;4   |

        And the ways
            | nodes | oneway |
            | abcd  | no     |

        When I match I should get
            | trace | code           |
            | abcd  | InvalidOptions |

    Scenario: Matching fail with waypoints param missing start/end
        Given the node map
            """
            a-----b---c
                  |
                  |
                  d
                  |
                  |
                  e
            """
        And the ways
            | nodes | oneway |
            | abc   | no     |
            | bde   | no     |

        Given the query options
            | waypoints | 1;3   |

        When I match I should get
            | trace | code         |
            | abde  | InvalidValue |

    Scenario: Testbot - Map matching with outlier that has no candidate and waypoint parameter
        Given a grid size of 100 meters
        Given the node map
            """
            a b c d

                1
            """

        And the ways
            | nodes | oneway |
            | abcd  | no     |

        Given the query options
            | waypoints | 0;2;3   |

        When I match I should get
            | trace | timestamps | code    |
            | ab1d  | 0 1 2 3    | NoMatch |

    Scenario: Regression test - avoid collapsing legs of a tidied split trace
        Given a grid size of 20 meters
        Given the node map
            """
            a--b--f
               |
               |
               e--c---d--g
            """
        Given the query options
        | tidy | true |

        And the ways
            | nodes | oneway |
            | abf   | no     |
            | be    | no     |
            | ecdg  | no     |

        When I match I should get
        | trace    | timestamps                                   | matchings  | code |
        | abbecd   | 10 11 27 1516914902 1516914913 1516914952    | ab,ecd     | Ok   |

    Scenario: Regression test - waypoints trimming too much geometry
        # fixes bug in map matching collapsing that was dropping path geometries
        # after segments that had 0 distance in internal route results
        Given the node map
            """
            ad
            |
            |
            |
            |
            |e   g
            b--------------c
            f              h
            """

        And the ways
            | nodes |
            | ab    |
            | bc    |

        Given the query options
            | waypoints | 0;4   |
            | overview  | full  |

        When I match I should get
            | trace    | geometry                           | code |
            | defgh    | 1,1,1,0.999461,1.000674,0.999461   | Ok   |

    @match @testbot
    Scenario: Regression test - waypoints trimming too much geometry
        Given the profile "testbot"
        Given a grid size of 10 meters
        Given the query options
          | geometries | geojson |
        Given the node map
          """
            bh
             |
             |
             |
             c
             g\
               \k
                \
                 \
                  \
                 j f
          """
        And the ways
          | nodes |
          | hc    |
          | cf    |
        Given the query options
          | waypoints | 0;3  |
          | overview  | full |
        When I match I should get
          | trace | geometry                                      | code |
          | bgkj  | 1.000135,1,1.000135,0.99964,1.000387,0.999137 | Ok   |


    @match @testbot
    # Regression test for issue #4919
    Scenario: Regression test - non-uturn maneuver preferred over uturn
        Given the profile "testbot"
        Given a grid size of 10 meters
        Given the query options
          | geometries | geojson |
        Given the node map
          """
                e
                ;
                ;
          a----hb-----c
                ;
                ;
                d
          """
        And the ways
          | nodes |
          | abc   |
          | dbe   |
        Given the query options
          | waypoints | 0;2  |
          | overview  | full |
          | steps     | true |
        When I match I should get
          | trace | geometry                                   | turns                    | code |
          | abc   | 1,0.99973,1.00027,0.99973,1.000539,0.99973 | depart,arrive            | Ok   |
          | abd   | 1,0.99973,1.00027,0.99973,1.00027,0.999461 | depart,turn right,arrive | Ok   |
          | abe   | 1,0.99973,1.00027,0.99973,1.00027,1        | depart,turn left,arrive  | Ok   |
          | ahd   | 1,0.99973,1.00027,0.99973,1.00027,0.999461 | depart,turn right,arrive | Ok   |
          | ahe   | 1,0.99973,1.00027,0.99973,1.00027,1        | depart,turn left,arrive  | Ok   |

    @match @testbot
    Scenario: Regression test - add source phantoms properly (one phantom on one edge)
        Given the profile "testbot"
        Given a grid size of 10 meters
        Given the node map
          """
          a--1-b2-cd3--e
          """
        And the ways
          | nodes |
          | ab    |
          | bcd   |
          | de    |
        Given the query options
          | geometries     | geojson         |
          | overview       | full            |
          | steps          | true            |
          | waypoints      | 0;2             |
          | annotations    | duration,weight |
          | generate_hints | false           |
        When I match I should get
          | trace | geometry                                             | a:duration    | a:weight      | duration |
          | 123   | 1.000135,1,1.000225,1,1.00036,1,1.000405,1,1.00045,1 | 1:1.5:0.5:0.5 | 1:1.5:0.5:0.5 | 3.5      |
          | 321   | 1.00045,1,1.000405,1,1.00036,1,1.000225,1,1.000135,1 | 0.5:0.5:1.5:1 | 0.5:0.5:1.5:1 | 3.5      |

    @match @testbot
    Scenario: Regression test - add source phantom properly (two phantoms on one edge)
        Given the profile "testbot"
        Given a grid size of 10 meters
        Given the node map
          """
          a--1-b23-c4--d
          """
        And the ways
          | nodes |
          | ab    |
          | bc    |
          | cd    |
        Given the query options
          | geometries     | geojson         |
          | overview       | full            |
          | steps          | true            |
          | waypoints      | 0;3             |
          | annotations    | duration,weight |
          | generate_hints | false           |
        When I match I should get
          | trace | geometry                                   | a:duration | a:weight | duration |
          | 1234  | 1.000135,1,1.000225,1,1.000405,1,1.00045,1 | 1:2:0.5    | 1:2:0.5  | 3.5      |
          | 4321  | 1.00045,1,1.000405,1,1.000225,1,1.000135,1 | 0.5:2:1    | 0.5:2:1  | 3.5      |

    @match @testbot
    Scenario: Regression test - add source phantom properly (two phantoms on one edge)
        Given the profile "testbot"
        Given a grid size of 10 meters
        Given the node map
          """
          a--12345-b
          """
        And the ways
          | nodes |
          | ab    |
        Given the query options
          | geometries     | geojson                  |
          | overview       | full                     |
          | steps          | true                     |
          | waypoints      | 0;3                      |
          | annotations    | duration,weight,distance |
          | generate_hints | false                    |

        # These should have the same weights/duration in either direction
        When I match I should get
          | trace | geometry             | a:distance | a:duration | a:weight | duration |
          | 2345  | 1.00018,1,1.000315,1 | 15.013264  | 1.5        | 1.5      | 1.5      |
          | 4321  | 1.00027,1,1.000135,1 | 15.013264  | 1.5        | 1.5      | 1.5      |