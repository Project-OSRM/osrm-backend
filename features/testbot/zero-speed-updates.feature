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
            | a,b,c,d   | abcda | NoTrips |
            | d,b,c,a   | dbcad | NoTrips |
