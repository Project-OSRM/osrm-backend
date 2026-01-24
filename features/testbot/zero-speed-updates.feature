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

        When I route I should get
          | from | to | code    |
          |    1 |  2 | NoRoute |


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


        When I route I should get
          | from | to | code    | alternative |
          |    1 |  2 | NoRoute |             |


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

        When I route I should get
          | waypoints | code    |
          | 1,2,3     | NoRoute |
          | 3,2,1     | NoRoute |


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
