Feature: Check zero speed updates

    Background:
        Given the profile "testbot"

    Scenario: Matching on restricted way, single segment
        Given the query options
            | geometries  | geojson |
            | annotations | true    |

        Given the node map
            """
            a-1--b--c-2-d
            """

        And the ways
            | nodes |
            | abcd  |
        And the contract extra arguments "--segment-speed-file {speeds_file}"
        And the customize extra arguments "--segment-speed-file {speeds_file}"
        And the speed file
            """
            2,3,0
            """

        When I match I should get
            | trace | code    |
            |    12 | NoMatch |


    Scenario: Matching restricted way, both segments
        Given the node map
            """
            a-1--b-2-c
            """

        And the ways
            | nodes | oneway |
            | abc   | no     |
        And the contract extra arguments "--segment-speed-file {speeds_file}"
        And the customize extra arguments "--segment-speed-file {speeds_file}"
        And the speed file
            """
            2,3,0
            3,2,0
            """

        When I match I should get
          | trace | code    |
          |    12 | NoMatch |


    Scenario: Matching on restricted oneway
        Given the node map
            """
            a-1--b-2-c
            """

        And the ways
            | nodes | oneway |
            | abc   | yes    |
        And the contract extra arguments "--segment-speed-file {speeds_file}"
        And the customize extra arguments "--segment-speed-file {speeds_file}"
        And the speed file
            """
            2,3,0
            """

        When I match I should get
            | trace | code    |
            |    12 | NoMatch |


    Scenario: Routing on restricted way
        Given the node map
            """
            a-1-b-2-c
            """

        And the ways
            | nodes | oneway |
            | abc   | no     |
        And the contract extra arguments "--segment-speed-file {speeds_file}"
        And the customize extra arguments "--segment-speed-file {speeds_file}"
        And the speed file
            """
            2,3,0
            3,2,0
            """

        # 2 is on the closed segment, so it snaps to b, which 1 can reach
        When I route I should get
          | from | to | route   | code |
          |    1 |  2 | abc,abc | Ok   |


    Scenario: Routing with alternatives on restricted way
        Given the node map
            """
            a-1-b-2-c
            """

        And the ways
            | nodes | oneway |
            | abc   | no     |
        And the contract extra arguments "--segment-speed-file {speeds_file}"
        And the customize extra arguments "--segment-speed-file {speeds_file}"
        And the speed file
            """
            1,2,0
            2,1,0
            """
        And the query options
            | alternatives | true |


        # 1 is on the closed segment, so it snaps to b, from where 2 is reachable
        When I route I should get
          | from | to | route   | code | alternative |
          |    1 |  2 | abc,abc | Ok   |             |


    Scenario: Routing on restricted oneway
        Given the node map
            """
            a-1-b-2-c
            """

        And the ways
            | nodes | oneway |
            | abc   | yes    |
        And the contract extra arguments "--segment-speed-file {speeds_file}"
        And the customize extra arguments "--segment-speed-file {speeds_file}"
        And the speed file
            """
            2,3,0
            3,2,0
            """

        When I route I should get
          | from | to | bearings | code      |
          |    1 |  2 | 270 270  | NoSegment |


    Scenario: Via routing on restricted oneway
        Given the node map
            """
            a-1-b-2-c-3-d
            """

        And the ways
            | nodes | oneway |
            | abc   | no     |
        And the contract extra arguments "--segment-speed-file {speeds_file}"
        And the customize extra arguments "--segment-speed-file {speeds_file}"
        And the speed file
            """
            2,3,0
            3,2,0
            """

        # 2 is on the closed segment and 3 past the end of the way, so both snap to b
        When I route I should get
          | waypoints | route           | code |
          | 1,2,3     | abc,abc,abc,abc | Ok   |
          | 3,2,1     | abc,abc,abc,abc | Ok   |


    @trip
    Scenario: Trip
        Given the node map
            """
            a b
            c d
            """

        And the ways
            | nodes |
            | ab    |
            | bc    |
            | cb    |
            | da    |
        And the contract extra arguments "--segment-speed-file {speeds_file}"
        And the customize extra arguments "--segment-speed-file {speeds_file}"
        And the speed file
            """
            1,2,0
            2,1,0
            """

        When I plan a trip I should get
            | waypoints | trips | code    |
            | a,b,c,d   |       | NoTrips |
            | d,b,c,a   |       | NoTrips |


    Scenario: Closing intersection should not block turns at earlier intersections
        Given the node map
            """
                x
                |
        a - 1 - b - 2 - c - 3 - d
                        |
                        y
            """

        And the ways
            | nodes |
            | abcd  |
            | xb    |
            | cy    |
        And the contract extra arguments "--segment-speed-file {speeds_file}"
        And the customize extra arguments "--segment-speed-file {speeds_file}"
        # Node IDs (top-to-bottom, left-to-right): x=1, a=2, b=3, c=4, d=5, y=6
        # Close ALL segments connecting to node c (4):
        # - b-c (3,4) and c-b (4,3)
        # - c-d (4,5) and d-c (5,4)
        # - c-y (4,6) and y-c (6,4)
        And the speed file
            """
            3,4,0
            4,3,0
            4,5,0
            5,4,0
            4,6,0
            6,4,0
            """

        # Route from position 1 (on open segment a-b) to x should work
        # because turn at b is not affected by closure at c
        When I route I should get
          | from | to | code |
          | 1    | x  | Ok   |
          | a    | x  | Ok   |


    Scenario: Closing segment should not cascade to parallel routes
        Given the node map
            """
            a - 1 - b - 2 - c
            |               |
            d - 3 - e - 4 - f
            """

        And the ways
            | nodes |
            | abc   |
            | def   |
            | ad    |
            | cf    |
        And the contract extra arguments "--segment-speed-file {speeds_file}"
        And the customize extra arguments "--segment-speed-file {speeds_file}"
        # Node IDs (top-to-bottom, left-to-right): a=1, b=2, c=3, d=4, e=5, f=6
        # Close segment b-c (2,3)
        And the speed file
            """
            2,3,0
            3,2,0
            """

        # Route on def should work, route a->f uses alternative via def
        When I route I should get
          | from | to | code |
          | 3    | 4  | Ok   |
          | a    | f  | Ok   |


    Scenario: Closing a segment should not trap a start past the closure on the same way
        Given the node map
            """
            x
            |
            a - 1 - 2 - b - 3 - c - d
                                |
                                y
            """

        And the ways
            | nodes |
            | abcd  |
            | xa    |
            | cy    |
        And the contract extra arguments "--segment-speed-file {speeds_file}"
        And the customize extra arguments "--segment-speed-file {speeds_file}"
        # Node IDs (top-to-bottom, left-to-right): x=1, a=2, b=3, c=4, d=5, y=6
        # Close b-c (3,4) in both directions. a-b-c is a single edge between the
        # intersections at a and c, with 1 and 2 on the open part of it.
        And the speed file
            """
            3,4,0
            4,3,0
            """

        When I route I should get
          | from | to | route      | code    |
          | 1    | 2  | abcd,abcd  | Ok      |
          | 2    | 1  | abcd,abcd  | Ok      |
          | x    | 1  | xa,abcd,abcd | Ok    |
          | 1    | d  |            | NoRoute |
          | 2    | y  |            | NoRoute |
          | x    | d  |            | NoRoute |
          | y    | 1  |            | NoRoute |

    # CH drops the turns out of a node with a closed segment, so it still has no way off it
    @no_ch
    Scenario: Closing a segment should not trap a start past the closure on the same way, MLD
        Given the node map
            """
            x
            |
            a - 1 - 2 - b - 3 - c - d
                                |
                                y
            """

        And the ways
            | nodes |
            | abcd  |
            | xa    |
            | cy    |
        And the contract extra arguments "--segment-speed-file {speeds_file}"
        And the customize extra arguments "--segment-speed-file {speeds_file}"
        And the speed file
            """
            3,4,0
            4,3,0
            """

        When I route I should get
          | from | to | route      | code    |
          | 1    | x  | abcd,xa,xa | Ok      |
          | 2    | x  | abcd,xa,xa | Ok      |

        # Arriving at 2 heading east, the way ahead is closed, so turn around there
        When I route I should get
          | waypoints | route                 | code |
          | 1,2,x     | abcd,abcd,abcd,xa,xa  | Ok   |

        When I request a travel time matrix I should get
          |   | 1  | 2  | x  |
          | 1 | 0  | 20 | 40 |
          | 2 | 20 | 0  | 60 |
          | x | 40 | 60 | 0  |


    Scenario: Turning around at a waypoint heading towards a closed segment
        Given the node map
            """
            x
            |
            a - 1 - 2 - 3 - b - c - d
                                |
                                y
            """

        And the ways
            | nodes |
            | abcd  |
            | xa    |
            | cy    |
        And the contract extra arguments "--segment-speed-file {speeds_file}"
        And the customize extra arguments "--segment-speed-file {speeds_file}"
        # Node IDs (top-to-bottom, left-to-right): x=1, a=2, b=3, c=4, d=5, y=6
        # Close b-c (3,4) in both directions: arriving at 3 heading east, the way
        # ahead is closed, so the route has to turn around there.
        And the speed file
            """
            3,4,0
            4,3,0
            """

        When I route I should get
          | waypoints | route               | time    | code |
          | 2,3,1     | abcd,abcd,abcd,abcd | 60s +-1 | Ok   |
          | 1,3,2     | abcd,abcd,abcd,abcd | 60s +-1 | Ok   |

        When I route I should get
          | waypoints | code    |
          | 1,3,y     | NoRoute |
