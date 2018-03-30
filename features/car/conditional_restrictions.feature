@routing @car @restrictions
Feature: Car - Turn restrictions
# Handle turn restrictions as defined by http://wiki.openstreetmap.org/wiki/Relation:restriction
# Note that if u-turns are allowed, turn restrictions can lead to suprising, but correct, routes.

    Background: Use car routing
        Given the profile "car"
        Given a grid size of 200 meters
        Given the origin -9.2972,10.3811
        # coordinate in Guinée, a country that observes GMT year round

    @no_turning @conditionals
    Scenario: Car - ignores unrecognized restriction
        Given the extract extra arguments "--parse-conditional-restrictions"
                                            # time stamp for 10am on Tues, 02 May 2017 GMT
        Given the contract extra arguments "--time-zone-file=test/data/tz/{timezone_names}/guinea.geojson --parse-conditionals-from-now=1493719200"
        Given the customize extra arguments "--time-zone-file=test/data/tz/{timezone_names}/guinea.geojson --parse-conditionals-from-now=1493719200"
        Given the node map
            """
              a
            d j b
              c
            """

        And the ways
            | nodes | oneway |
            | aj    | no     |
            | jc    | no     |
            | bj    | yes    |
            | jd    | yes    |

        And the relations
            | type        | way:from | way:to | node:via | restriction:conditional                |
            | restriction | bj       | aj     | j        | only_right_turn @ (has_pygmies > 10 p) |

        When I route I should get
            | from | to | route    |
            | b    | c  | bj,jc,jc |
            | b    | a  | bj,aj,aj |
            | b    | d  | bj,jd,jd |

    @no_turning @conditionals
    Scenario: Car - Restriction would be on, but the restriction was badly tagged
        Given the extract extra arguments "--parse-conditional-restrictions"
                                            # time stamp for 10am on Tues, 02 May 2017 GMT
        Given the contract extra arguments "--time-zone-file=test/data/tz/{timezone_names}/guinea.geojson --parse-conditionals-from-now=1493719200"
        Given the customize extra arguments "--time-zone-file=test/data/tz/{timezone_names}/guinea.geojson --parse-conditionals-from-now=1493719200"

        Given the node map
            """
              a
           p  |
            \ |
              j
              | \
              c  m
            """

        And the ways
            | nodes |
            | aj    |
            | jc    |
            | pjm   |

        And the relations
            | type        | way:from | way:to | node:via | restriction:conditional             |
            | restriction | aj       | pjm    | j        | no_left_turn @ (Mo-Fr 07:00-10:30)  |
            | restriction | jc       | pjm    | j        | no_right_turn @ (Mo-Fr 07:00-10:30) |

        When I route I should get
            | from | to | route      |
            | a    | m  | aj,pjm,pjm |
            | c    | m  | jc,pjm,pjm |

    @no_turning @conditionals
    Scenario: Car - Restriction With Compressed Geometry
        Given the extract extra arguments "--parse-conditional-restrictions"
                                            # time stamp for 10am on Tues, 02 May 2017 GMT
        Given the contract extra arguments "--time-zone-file=test/data/tz/{timezone_names}/guinea.geojson --parse-conditionals-from-now=1493719200"
        Given the customize extra arguments "--time-zone-file=test/data/tz/{timezone_names}/guinea.geojson --parse-conditionals-from-now=1493719200"

        Given the node map
            """
            n
            |
            i
            |
            j-k-l-m
            |
            s
            """

        And the ways
            | nodes |
            | nij   |
            | js    |
            | jklm  |

        And the relations
            | type        | way:from | way:to | node:via | restriction:conditional             |
            | restriction | nij      | jklm   | j        | no_left_turn @ (Mo-Fr 07:00-10:30)  |

        When I route I should get
            | from | to | route               |
            | n    | m  | nij,js,js,jklm,jklm |

    @no_turning @conditionals
    Scenario: Car - Restriction With Compressed Geometry and Traffic Signal
        Given the extract extra arguments "--parse-conditional-restrictions"
                                            # time stamp for 10am on Tues, 02 May 2017 GMT
        Given the contract extra arguments "--time-zone-file=test/data/tz/{timezone_names}/guinea.geojson --parse-conditionals-from-now=1493719200"
        Given the customize extra arguments "--time-zone-file=test/data/tz/{timezone_names}/guinea.geojson --parse-conditionals-from-now=1493719200"

        Given the node map
            """
            n
            |
            i
            |
            j-k-l-m
            |
            s
            """

        And the ways
            | nodes |
            | nij   |
            | js    |
            | jklm  |

        And the nodes
            | node | highway        |
            | i    | traffic_signal |
            | k    | traffic_signal |

        And the relations
            | type        | way:from | way:to | node:via | restriction:conditional             |
            | restriction | nij      | jklm   | j        | no_left_turn @ (Mo-Fr 07:00-10:30)  |

        When I route I should get
            | from | to | route               |
            | n    | m  | nij,js,js,jklm,jklm |

    @no_turning @conditionals
    Scenario: Car - ignores except restriction
        Given the extract extra arguments "--parse-conditional-restrictions"
                                            # time stamp for 10am on Tues, 02 May 2017 GMT
        Given the contract extra arguments "--time-zone-file=test/data/tz/{timezone_names}/guinea.geojson --parse-conditionals-from-now=1493719200"
        Given the customize extra arguments "--time-zone-file=test/data/tz/{timezone_names}/guinea.geojson --parse-conditionals-from-now=1493719200"
        Given the node map
            """
              a
            d j b
              c
            """

        And the ways
            | nodes | oneway |
            | aj    | no     |
            | jc    | no     |
            | bj    | no     |
            | jd    | no     |

        And the relations
            | type        | way:from | way:to | node:via | restriction:conditional               | except   |
            | restriction | bj       | aj     | j        | only_right_turn @ (Mo-Su 08:00-12:00) | motorcar |
            | restriction | jd       | aj     | j        | only_left_turn @ (Mo-Su 08:00-12:00)  | bus      |

        When I route I should get
            | from | to | route          | # |
            | b    | c  | bj,jc,jc       |   |
            | b    | a  | bj,aj,aj       | restriction does not apply to cars |
            | b    | d  | bj,jd,jd       |   |
            | d    | c  | jd,aj,aj,jc,jc | restriction excepting busses still applies to cars  |

    @no_turning @conditionals
    Scenario: Car - only_right_turn
        Given the extract extra arguments "--parse-conditional-restrictions"
                                            # time stamp for 10am on Tues, 02 May 2017 GMT
        Given the contract extra arguments "--time-zone-file=test/data/tz/{timezone_names}/guinea.geojson --parse-conditionals-from-now=1493719200"
        Given the customize extra arguments "--time-zone-file=test/data/tz/{timezone_names}/guinea.geojson --parse-conditionals-from-now=1493719200"
        Given the node map
            """
              a
            d j b
              c
            """

        And the ways
            | nodes | oneway |
            | aj    | no     |
            | jc    | no     |
            | bj    | yes    |
            | jd    | yes    |

        And the relations
            | type        | way:from | way:to | node:via | restriction:conditional               |
            | restriction | bj       | aj     | j        | only_right_turn @ (Mo-Su 07:00-14:00) |

        When I route I should get
            | from | to | route          |
            | b    | c  | bj,aj,aj,jc,jc |
            | b    | a  | bj,aj,aj       |
            | b    | d  | bj,aj,aj,jd,jd |

    @no_turning @conditionals
    Scenario: Car - No right turn
        Given the extract extra arguments "--parse-conditional-restrictions"
                                            # time stamp for 10am on Tues, 02 May 2017 GMT
        Given the contract extra arguments "--time-zone-file=test/data/tz/{timezone_names}/guinea.geojson --parse-conditionals-from-now=1493719200"
        Given the customize extra arguments "--time-zone-file=test/data/tz/{timezone_names}/guinea.geojson --parse-conditionals-from-now=1493719200"
        Given the node map
            """
              a
            d j b
              c
            """

        And the ways
            | nodes | oneway |
            | aj    | no     |
            | jc    | no     |
            | bj    | yes    |
            | jd    | yes    |

        And the relations
            | type        | way:from | way:to | node:via | restriction:conditional               |
            | restriction | bj       | aj     | j        | no_right_turn @ (Mo-Fr 07:00-13:00)   |

        When I route I should get
            | from | to | route          | # |
            | b    | c  | bj,jc,jc       | normal turn |
            | b    | a  | bj,jc,jc,aj,aj | avoids right turn |
            | b    | d  | bj,jd,jd       | normal maneuver |

    @only_turning @conditionals
    Scenario: Car - only_left_turn
        Given the extract extra arguments "--parse-conditional-restrictions"
                                            # time stamp for 10am on Tues, 02 May 2017 GMT
        Given the contract extra arguments "--time-zone-file=test/data/tz/{timezone_names}/guinea.geojson --parse-conditionals-from-now=1493719200"
        Given the customize extra arguments "--time-zone-file=test/data/tz/{timezone_names}/guinea.geojson --parse-conditionals-from-now=1493719200"
        Given the node map
            """
              a
            d j b
              c
            """

        And the ways
            | nodes | oneway |
            | aj    | no     |
            | jc    | no     |
            | bj    | yes    |
            | jd    | yes    |

        And the relations
            | type        | way:from | way:to | node:via | restriction:conditional              |
            | restriction | bj       | jc     | j        | only_left_turn @ (Mo-Fr 07:00-16:00) |

        When I route I should get
            | from | to | route          |
            | b    | c  | bj,jc,jc       |
            | b    | a  | bj,jc,jc,aj,aj |
            | b    | d  | bj,jc,jc,jd,jd |

    @no_turning @conditionals
    Scenario: Car - No left turn
        Given the extract extra arguments "--parse-conditional-restrictions"
                                            # time stamp for 10am on Tues, 02 May 2017 GMT
        Given the contract extra arguments "--time-zone-file=test/data/tz/{timezone_names}/guinea.geojson --parse-conditionals-from-now=1493719200"
        Given the customize extra arguments "--time-zone-file=test/data/tz/{timezone_names}/guinea.geojson --parse-conditionals-from-now=1493719200"
        Given the node map
            """
              a
            d j b
              c
            """

        And the ways
            | nodes | oneway |
            | aj    | no     |
            | jc    | no     |
            | bj    | yes    |
            | jd    | yes    |

        And the relations
            | type        | way:from | way:to | node:via | restriction:conditional            |
            | restriction | bj       | jc     | j        | no_left_turn @ (Mo-Su 00:00-23:59) |

        When I route I should get
            | from | to | route          |
            | b    | c  | bj,aj,aj,jc,jc |
            | b    | a  | bj,aj,aj       |
            | b    | d  | bj,jd,jd       |

    @no_turning @conditionals
    Scenario: Car - Conditional restriction is off
        Given the extract extra arguments "--parse-conditional-restrictions"
                                            # time stamp for 10am on Tues, 02 May 2017 GMT
        Given the contract extra arguments "--time-zone-file=test/data/tz/{timezone_names}/guinea.geojson --parse-conditionals-from-now=1493719200"
        Given the customize extra arguments "--time-zone-file=test/data/tz/{timezone_names}/guinea.geojson --parse-conditionals-from-now=1493719200"
        Given the node map
            """
              a
            d j b
              c
            """

        And the ways
            | nodes | oneway |
            | aj    | no     |
            | jc    | no     |
            | bj    | yes    |
            | jd    | yes    |

        And the relations
            | type        | way:from | way:to | node:via | restriction:conditional             |
            | restriction | bj       | aj     | j        | no_right_turn @ (Mo-Su 16:00-20:00) |

        When I route I should get
            | from | to | route    |
            | b    | c  | bj,jc,jc |
            | b    | a  | bj,aj,aj |
            | b    | d  | bj,jd,jd |

    @no_turning @conditionals
    Scenario: Car - Conditional restriction is on
        Given the extract extra arguments "--parse-conditional-restrictions"
        # 10am utc, wed
        Given the contract extra arguments "--time-zone-file=test/data/tz/{timezone_names}/guinea.geojson --parse-conditionals-from-now=1493805600"
        Given the customize extra arguments "--time-zone-file=test/data/tz/{timezone_names}/guinea.geojson --parse-conditionals-from-now=1493805600"
        Given the node map
            """
              a
            d j b
              c
            """

        And the ways
            | nodes | oneway |
            | aj    | no     |
            | jc    | no     |
            | bj    | yes    |
            | jd    | yes    |

        And the relations
            | type        | way:from | way:to | node:via | restriction:conditional             |
            | restriction | bj       | aj     | j        | no_right_turn @ (Mo-Fr 07:00-14:00) |

        When I route I should get
            | from | to | route          |
            | b    | c  | bj,jc,jc       |
            | b    | a  | bj,jc,jc,aj,aj |
            | b    | d  | bj,jd,jd       |

    @no_turning @conditionals
    Scenario: Car - Conditional restriction with multiple time windows
        Given the extract extra arguments "--parse-conditional-restrictions"
        # 5pm Wed 02 May, 2017 GMT
        Given the contract extra arguments "--time-zone-file=test/data/tz/{timezone_names}/guinea.geojson --parse-conditionals-from-now=1493744400"
        Given the customize extra arguments "--time-zone-file=test/data/tz/{timezone_names}/guinea.geojson --parse-conditionals-from-now=1493744400"

        Given the node map
            """
              a
           p  |
            \ |
              j
              | \
              c  m
            """

        And the ways
            | nodes | oneway |
            | aj    | no     |
            | jc    | no     |
            | jp    | yes    |
            | mj    | yes    |

        And the relations
            | type        | way:from | way:to | node:via | restriction:conditional                         |
            | restriction | aj       | jp     | j        | no_right_turn @ (Mo-Fr 07:00-11:00,16:00-18:30) |

        When I route I should get
            | from | to | route          |
            | a    | p  | aj,jc,jc,jp,jp |
            | m    | p  | mj,jp,jp       |

    @no_turning @conditionals
    Scenario: Car - only_right_turn
        Given the extract extra arguments "--parse-conditional-restrictions"
                                            # time stamp for 10am on Tues, 02 May 2017 GMT
        Given the contract extra arguments "--time-zone-file=test/data/tz/{timezone_names}/guinea.geojson --parse-conditionals-from-now=1493719200"
        Given the customize extra arguments "--time-zone-file=test/data/tz/{timezone_names}/guinea.geojson --parse-conditionals-from-now=1493719200"
        Given the node map
            """
              a
            d j b
              c
            """

        And the ways
            | nodes | oneway |
            | aj    | no     |
            | jc    | no     |
            | bj    | yes    |
            | jd    | yes    |

        And the relations
            | type        | way:from | way:to | node:via | restriction:conditional               |
            | restriction | bj       | aj     | j        | only_right_turn @ (Mo-Su 07:00-14:00) |

        When I route I should get
            | from | to | route          |
            | b    | c  | bj,aj,aj,jc,jc |
            | b    | a  | bj,aj,aj       |
            | b    | d  | bj,aj,aj,jd,jd |

    @no_turning @conditionals
    Scenario: Car - No right turn
        Given the extract extra arguments "--parse-conditional-restrictions"
                                            # time stamp for 10am on Tues, 02 May 2017 GMT
        Given the contract extra arguments "--time-zone-file=test/data/tz/{timezone_names}/guinea.geojson --parse-conditionals-from-now=1493719200"
        Given the customize extra arguments "--time-zone-file=test/data/tz/{timezone_names}/guinea.geojson --parse-conditionals-from-now=1493719200"
        Given the node map
            """
              a
            d j b
              c
            """

        And the ways
            | nodes | oneway |
            | aj    | no     |
            | jc    | no     |
            | bj    | yes    |
            | jd    | yes    |

        And the relations
            | type        | way:from | way:to | node:via | restriction:conditional               |
            | restriction | bj       | aj     | j        | no_right_turn @ (Mo-Fr 07:00-13:00)   |

        When I route I should get
            | from | to | route          | # |
            | b    | c  | bj,jc,jc       | normal turn |
            | b    | a  | bj,jc,jc,aj,aj | avoids right turn |
            | b    | d  | bj,jd,jd       | normal maneuver |

    @only_turning @conditionals
    Scenario: Car - only_left_turn
        Given the extract extra arguments "--parse-conditional-restrictions"
                                            # time stamp for 10am on Tues, 02 May 2017 GMT
        Given the contract extra arguments "--time-zone-file=test/data/tz/{timezone_names}/guinea.geojson --parse-conditionals-from-now=1493719200"
        Given the customize extra arguments "--time-zone-file=test/data/tz/{timezone_names}/guinea.geojson --parse-conditionals-from-now=1493719200"
        Given the node map
            """
              a
            d j b
              c
            """

        And the ways
            | nodes | oneway |
            | aj    | no     |
            | jc    | no     |
            | bj    | yes    |
            | jd    | yes    |

        And the relations
            | type        | way:from | way:to | node:via | restriction:conditional              |
            | restriction | bj       | jc     | j        | only_left_turn @ (Mo-Fr 07:00-16:00) |

        When I route I should get
            | from | to | route          |
            | b    | c  | bj,jc,jc       |
            | b    | a  | bj,jc,jc,aj,aj |
            | b    | d  | bj,jc,jc,jd,jd |

    @no_turning @conditionals
    Scenario: Car - No left turn
        Given the extract extra arguments "--parse-conditional-restrictions"
                                            # time stamp for 10am on Tues, 02 May 2017 GMT
        Given the contract extra arguments "--time-zone-file=test/data/tz/{timezone_names}/guinea.geojson --parse-conditionals-from-now=1493719200"
        Given the customize extra arguments "--time-zone-file=test/data/tz/{timezone_names}/guinea.geojson --parse-conditionals-from-now=1493719200"
        Given the node map
            """
              a
            d j b
              c
            """

        And the ways
            | nodes | oneway |
            | aj    | no     |
            | jc    | no     |
            | bj    | yes    |
            | jd    | yes    |

        And the relations
            | type        | way:from | way:to | node:via | restriction:conditional            |
            | restriction | bj       | jc     | j        | no_left_turn @ (Mo-Su 00:00-23:59) |

        When I route I should get
            | from | to | route          |
            | b    | c  | bj,aj,aj,jc,jc |
            | b    | a  | bj,aj,aj       |
            | b    | d  | bj,jd,jd       |

    @no_turning @conditionals
    Scenario: Car - Conditional restriction is off
        Given the extract extra arguments "--parse-conditional-restrictions"
                                            # time stamp for 10am on Tues, 02 May 2017 GMT
        Given the contract extra arguments "--time-zone-file=test/data/tz/{timezone_names}/guinea.geojson --parse-conditionals-from-now=1493719200"
        Given the customize extra arguments "--time-zone-file=test/data/tz/{timezone_names}/guinea.geojson --parse-conditionals-from-now=1493719200"
        Given the node map
            """
              a
            d j b
              c
            """

        And the ways
            | nodes | oneway |
            | aj    | no     |
            | jc    | no     |
            | bj    | yes    |
            | jd    | yes    |

        And the relations
            | type        | way:from | way:to | node:via | restriction:conditional             |
            | restriction | bj       | aj     | j        | no_right_turn @ (Mo-Su 16:00-20:00) |

        When I route I should get
            | from | to | route    |
            | b    | c  | bj,jc,jc |
            | b    | a  | bj,aj,aj |
            | b    | d  | bj,jd,jd |

    @no_turning @conditionals
    Scenario: Car - Conditional restriction is on
        Given the extract extra arguments "--parse-conditional-restrictions"
        # 10am utc, wed
        Given the contract extra arguments "--time-zone-file=test/data/tz/{timezone_names}/guinea.geojson --parse-conditionals-from-now=1493805600"
        Given the customize extra arguments "--time-zone-file=test/data/tz/{timezone_names}/guinea.geojson --parse-conditionals-from-now=1493805600"
        Given the node map
            """
              a
            d j b
              c
            """

        And the ways
            | nodes | oneway |
            | aj    | no     |
            | jc    | no     |
            | bj    | yes    |
            | jd    | yes    |

        And the relations
            | type        | way:from | way:to | node:via | restriction:conditional             |
            | restriction | jb       | aj     | j        | no_right_turn @ (Mo-Fr 07:00-14:00) |

        When I route I should get
            | from | to | route          |
            | b    | c  | bj,jc,jc       |
            | b    | a  | bj,jc,jc,aj,aj |
            | b    | d  | bj,jd,jd       |

    @no_turning @conditionals
    Scenario: Car - Conditional restriction with multiple time windows
        Given the extract extra arguments "--parse-conditional-restrictions"
        # 5pm Wed 02 May, 2017 GMT
        Given the contract extra arguments "--time-zone-file=test/data/tz/{timezone_names}/guinea.geojson --parse-conditionals-from-now=1493744400"
        Given the customize extra arguments "--time-zone-file=test/data/tz/{timezone_names}/guinea.geojson --parse-conditionals-from-now=1493744400"

        Given the node map
            """
              a
           p  |
            \ |
              j
              | \
              c  m
            """

        And the ways
            | nodes | oneway |
            | aj    | no     |
            | jc    | no     |
            | jp    | yes    |
            | mj    | yes    |

        And the relations
            | type        | way:from | way:to | node:via | restriction:conditional                         |
            | restriction | aj       | jp     | j        | no_right_turn @ (Mo-Fr 07:00-11:00,16:00-18:30) |

        When I route I should get
            | from | to | route          |
            | a    | p  | aj,jc,jc,jp,jp |
            | m    | p  | mj,jp,jp       |

    @restriction-way
    Scenario: Car - prohibit turn
        Given the extract extra arguments "--parse-conditional-restrictions"
        # 5pm Wed 02 May, 2017 GMT
        Given the contract extra arguments "--time-zone-file=test/data/tz/{timezone_names}/guinea.geojson --parse-conditionals-from-now=1493744400"
        Given the customize extra arguments "--time-zone-file=test/data/tz/{timezone_names}/guinea.geojson --parse-conditionals-from-now=1493744400"

        Given the node map
            """
            c
            |
            |   f
            |   |
            b---e
            |   |
            a   d
            """

        And the ways
            | nodes |
            | ab    |
            | bc    |
            | be    |
            | de    |
            | ef    |

        And the relations
            | type        | way:from | way:via | way:to | restriction:conditional                         |
            | restriction | ab       | be      | de     | no_right_turn @ (Mo-Fr 07:00-11:00,16:00-18:30) |

        When I route I should get
            | from | to | route             | turns                                                               | locations   |
            | a    | d  | ab,be,ef,ef,de,de | depart,turn right,turn left,continue uturn,new name straight,arrive | a,b,e,f,e,d |
            | a    | f  | ab,be,ef,ef       | depart,turn right,turn left,arrive                                  | a,b,e,f     |
            | c    | d  | bc,be,de,de       | depart,turn left,turn right,arrive                                  | c,b,e,d     |
            | c    | f  | bc,be,ef,ef       | depart,turn left,turn left,arrive                                   | c,b,e,f     |

    # condition is off
    @restriction-way
    Scenario: Car - prohibit turn
        Given the extract extra arguments "--parse-conditional-restrictions"
        # time stamp for 12am on Tues, 02 May 2017 GMT
        Given the contract extra arguments "--time-zone-file=test/data/tz/{timezone_names}/guinea.geojson --parse-conditionals-from-now=1493726400"
        Given the customize extra arguments "--time-zone-file=test/data/tz/{timezone_names}/guinea.geojson --parse-conditionals-from-now=1493726400"

        Given the node map
            """
            c
            |
            |   f
            |   |
            b---e
            |   |
            a   d
            """

        And the ways
            | nodes |
            | ab    |
            | bc    |
            | be    |
            | de    |
            | ef    |

        And the relations
            | type        | way:from | way:via | way:to | restriction:conditional                         |
            | restriction | ab       | be      | de     | no_right_turn @ (Mo-Fr 07:00-11:00,16:00-18:30) |

        When I route I should get
            | from | to | route       |
            | a    | d  | ab,be,de,de |
            | a    | f  | ab,be,ef,ef |
            | c    | d  | bc,be,de,de |
            | c    | f  | bc,be,ef,ef |

    # https://www.openstreetmap.org/#map=18/38.91099/-77.00888
    @no_turning @conditionals
    Scenario: Car - DC North capitol situation, two on one off
        Given the extract extra arguments "--parse-conditional-restrictions"
        # 9pm Wed 02 May, 2017 UTC, 5pm EDT
        Given the contract extra arguments "--time-zone-file=test/data/tz/{timezone_names}/dc.geojson --parse-conditionals-from-now=1493845200"
        Given the customize extra arguments "--time-zone-file=test/data/tz/{timezone_names}/dc.geojson --parse-conditionals-from-now=1493845200"

        #    """
        #      a h
        #   d
        #      b g
        #           e
        #      c f
        #    """
        Given the node locations
            | node | lat     | lon      |
            | a    | 38.91124 | -77.00909 |
            | b    | 38.91080 | -77.00909 |
            | c    | 38.91038 | -77.00909 |
            | d    | 38.91105 | -77.00967 |
            | e    | 38.91037 | -77.00807 |
            | f    | 38.91036 | -77.00899 |
            | g    | 38.91076 | -77.00901 |
            | h    | 38.91124 | -77.00900 |

        And the ways
            | nodes | oneway | name       |
            | ab    | yes    | cap south  |
            | bc    | yes    | cap south  |
            | fg    | yes    | cap north  |
            | gh    | yes    | cap north  |
            | db    | no     | florida nw |
            | bg    | no     | florida    |
            | ge    | no     | florida ne |

        And the relations
            | type        | way:from  | way:to  | node:via | restriction:conditional                        |
            | restriction | ab        | bg      | b        | no_left_turn @ (Mo-Fr 07:00-09:30,16:00-18:30) |
            | restriction | fg        | bg      | g        | no_left_turn @ (Mo-Fr 06:00-10:00)             |
            | restriction | bg        | bc      | b        | no_left_turn @ (Mo-Fr 07:00-09:30,16:00-18:30) |

        When I route I should get
            | from | to | route                                      | turns                                       |
            | a    | e  | cap south,florida nw,florida nw,florida ne | depart,turn right,continue uturn,arrive     |
            | f    | d  | cap north,florida nw,florida nw               | depart,turn left,arrive                     |
            | e    | c  | florida ne,florida nw,cap south,cap south  | depart,continue uturn,turn right,arrive     |

    @no_turning @conditionals
    Scenario: Car - DC North capitol situation, one on two off
        Given the extract extra arguments "--parse-conditional-restrictions"
        # 10:30am utc, wed, 6:30am est
        Given the contract extra arguments "--time-zone-file=test/data/tz/{timezone_names}/dc.geojson --parse-conditionals-from-now=1493807400"
        Given the customize extra arguments "--time-zone-file=test/data/tz/{timezone_names}/dc.geojson --parse-conditionals-from-now=1493807400"

        #    """
        #      a h
        #   d
        #      b g
        #           e
        #      c f
        #    """
        Given the node locations
            | node | lat     | lon      |
            | a    | 38.91124 | -77.00909 |
            | b    | 38.91080 | -77.00909 |
            | c    | 38.91038 | -77.00909 |
            | d    | 38.91105 | -77.00967 |
            | e    | 38.91037 | -77.00807 |
            | f    | 38.91036 | -77.00899 |
            | g    | 38.91076 | -77.00901 |
            | h    | 38.91124 | -77.00900 |

        And the ways
            | nodes | oneway | name       |
            | ab    | yes    | cap south  |
            | bc    | yes    | cap south  |
            | fg    | yes    | cap north  |
            | gh    | yes    | cap north  |
            | db    | no     | florida nw |
            | bg    | no     | florida    |
            | ge    | no     | florida ne |

        And the relations
            | type        | way:from  | way:to  | node:via | restriction:conditional                        |
            | restriction | ab        | bg      | b        | no_left_turn @ (Mo-Fr 07:00-09:30,16:00-18:30) |
            | restriction | fg        | bg      | g        | no_left_turn @ (Mo-Fr 06:00-10:00)             |
            | restriction | bg        | bc      | b        | no_left_turn @ (Mo-Fr 07:00-09:30,16:00-18:30) |

        When I route I should get
            | from | to | route                                      | turns                                         |
            | a    | e  | cap south,florida ne,florida ne            | depart,turn left,arrive                       |
            | f    | d  | cap north,florida ne,florida ne,florida nw | depart,turn sharp right,continue uturn,arrive |
            | e    | c  | florida ne,cap south,cap south             | depart,turn left,arrive                       |

    @only_turning @conditionals
    Scenario: Car - Restriction is always off when point not found in timezone files
        # same test as the following one, but given a different time zone file
        Given the extract extra arguments "--parse-conditional-restrictions"
        # 9am UTC, 10am BST
        Given the contract extra arguments "--time-zone-file=test/data/tz/{timezone_names}/dc.geojson --parse-conditionals-from-now=1493802000"
        Given the customize extra arguments "--time-zone-file=test/data/tz/{timezone_names}/dc.geojson --parse-conditionals-from-now=1493802000"

        #    """
        #     a
        #          e
        #      b
        #   d
        #       c
        #    """
        Given the node locations
            | node | lat     | lon     |
            | a    | 51.5250 | -0.1166 |
            | b    | 51.5243 | -0.1159 |
            | c    | 51.5238 | -0.1152 |
            | d    | 51.5241 | -0.1167 |
            | e    | 51.5247 | -0.1153 |

        And the ways
            | nodes | name  |
            | ab    | albic |
            | bc    | albic |
            | db    | dobe |
            | be    | dobe |

        And the relations
            | type        | way:from  | way:to  | node:via | restriction:conditional               |
            | restriction | ab        | be      | b        | only_left_turn @ (Mo-Fr 07:00-11:00)  |

        When I route I should get
            | from | to | route           | turns                   |
            | a    | c  | albic,albic     | depart,arrive           |
            | a    | e  | albic,dobe,dobe | depart,turn left,arrive |

    @only_turning @conditionals
    Scenario: Car - Somewhere in london, the UK, GMT timezone
        Given the extract extra arguments "--parse-conditional-restrictions"
        # 9am UTC, 10am BST
        Given the contract extra arguments "--time-zone-file=test/data/tz/{timezone_names}/london.geojson --parse-conditionals-from-now=1493802000"
        Given the customize extra arguments "--time-zone-file=test/data/tz/{timezone_names}/london.geojson --parse-conditionals-from-now=1493802000"

        #    """
        #     a
        #          e
        #      b
        #   d
        #       c
        #    """
        Given the node locations
            | node | lat     | lon     |
            | a    | 51.5250 | -0.1166 |
            | b    | 51.5243 | -0.1159 |
            | c    | 51.5238 | -0.1152 |
            | d    | 51.5241 | -0.1167 |
            | e    | 51.5247 | -0.1153 |

        And the ways
            | nodes | name  |
            | ab    | albic |
            | bc    | albic |
            | db    | dobe |
            | be    | dobe |

        And the relations
            | type        | way:from  | way:to  | node:via | restriction:conditional               |
            | restriction | ab        | be      | b        | only_left_turn @ (Mo-Fr 07:00-11:00)  |

        When I route I should get
            | from | to | route                       | turns                                            |
            | a    | c  | albic,dobe,dobe,albic,albic | depart,turn left,continue uturn,turn left,arrive |
            | a    | e  | albic,dobe,dobe             | depart,turn left,arrive                          |

    @only_turning @conditionals
    Scenario: Car - Somewhere in London, the UK, GMT timezone
        Given the extract extra arguments "--parse-conditional-restrictions"
        # 9am UTC, 10am BST
        Given the contract extra arguments "--time-zone-file=test/data/tz/{timezone_names}/london.geojson --parse-conditionals-from-now=1493802000"
        Given the customize extra arguments "--time-zone-file=test/data/tz/{timezone_names}/london.geojson --parse-conditionals-from-now=1493802000"

        #    """
        #     a
        #          e
        #      b
        #   d
        #       c
        #    """
        Given the node locations
            | node | lat     | lon     |
            | a    | 51.5250 | -0.1166 |
            | b    | 51.5243 | -0.1159 |
            | c    | 51.5238 | -0.1152 |
            | d    | 51.5241 | -0.1167 |
            | e    | 51.5247 | -0.1153 |

        And the ways
            | nodes | name  |
            | ab    | albic |
            | bc    | albic |
            | db    | dobe |
            | be    | dobe |

        And the relations
            | type        | way:from  | way:to  | node:via | restriction:conditional               |
            | restriction | ab        | be      | b        | only_left_turn @ (Mo-Fr 07:00-11:00)  |

        When I route I should get
            | from | to | route                       | turns                                            |
            | a    | c  | albic,dobe,dobe,albic,albic | depart,turn left,continue uturn,turn left,arrive |
            | a    | e  | albic,dobe,dobe             | depart,turn left,arrive                          |

    @no_turning @conditionals @restriction-way
    Scenario: Car - Conditional restriction with multiple time windows
        Given the extract extra arguments "--parse-conditional-restrictions"
        # 5pm Wed 02 May, 2017 GMT
        Given the contract extra arguments "--time-zone-file=test/data/tz/{timezone_names}/guinea.geojson --parse-conditionals-from-now=1493744400"
        Given the customize extra arguments "--time-zone-file=test/data/tz/{timezone_names}/guinea.geojson --parse-conditionals-from-now=1493744400"

        Given the node map
            """
            a   f
            |   |
            b - e - h
            |   |   |
            c   d - g
                  1
            """

        And the ways
            | nodes |
            | ab    |
            | bc    |
            | de    |
            | ef    |
            | be    |
            | eh    |
            | gh    |
            | dg    |

        And the relations
            | type        | way:from | way:via | way:to | restriction:conditional                    |
            | restriction | ab       | be      | ef     | no_uturn @ (Mo-Fr 07:00-11:00,16:00-18:30) |

        And the relations
            | type        | way:from | way:to | node:via | restriction:conditional                    |
            | restriction | ed       | dg     | d        | no_uturn @ (Mo-Fr 07:00-11:00,16:00-18:30) |

        When I route I should get
            | from | to | route             |
            | a    | f  | ab,bc,bc,be,ef,ef |
            | f    | 1  | ef,eh,gh,dg,dg    |

    @restriction-way @overlap
    Scenario: Car - prohibit turn
        Given the extract extra arguments "--parse-conditional-restrictions"
        # 5pm Wed 02 May, 2017 GMT
        Given the contract extra arguments "--time-zone-file=test/data/tz/{timezone_names}/guinea.geojson --parse-conditionals-from-now=1493744400"
        Given the customize extra arguments "--time-zone-file=test/data/tz/{timezone_names}/guinea.geojson --parse-conditionals-from-now=1493744400"

        Given the node map
            """
            c
            |
            |   f
            |   |
            b---e
            |   |
            a   d
            """

        And the ways
            | nodes |
            | ab    |
            | bc    |
            | be    |
            | de    |
            | ef    |

        And the relations
            | type        | way:from | way:via | way:to | restriction:conditional             |
            | restriction | ab       | be      | de     | no_right_turn @ (Mo-Fr 07:00-11:00) |

        And the relations
            | type        | way:from | way:via | way:to | restriction   |
            | restriction | ab       | be      | de     | no_right_turn |

        # condition is off, but the general restriction should take precedence
        When I route I should get
            | from | to | route             | turns                                                               | locations   |
            | a    | d  | ab,be,ef,ef,de,de | depart,turn right,turn left,continue uturn,new name straight,arrive | a,b,e,f,e,d |
            | a    | f  | ab,be,ef,ef       | depart,turn right,turn left,arrive                                  | a,b,e,f     |
            | c    | d  | bc,be,de,de       | depart,turn left,turn right,arrive                                  | c,b,e,d     |
            | c    | f  | bc,be,ef,ef       | depart,turn left,turn left,arrive                                   | c,b,e,f     |

    @restriction-way @overlap
    Scenario: Car - prohibit turn
        Given the extract extra arguments "--parse-conditional-restrictions"
        # 5pm Wed 02 May, 2017 GMT
        Given the contract extra arguments "--time-zone-file=test/data/tz/{timezone_names}/guinea.geojson --parse-conditionals-from-now=1493744400"
        Given the customize extra arguments "--time-zone-file=test/data/tz/{timezone_names}/guinea.geojson --parse-conditionals-from-now=1493744400"

        Given the node map
            """
            c
            |
            |   f
            |   |
            b---e
            |   |
            |   d
            |
            a
            """

        And the ways
            | nodes |
            | ab    |
            | bc    |
            | be    |
            | de    |
            | ef    |

        And the relations
            | type        | way:from | way:via | way:to | restriction:conditional             |
            | restriction | ab       | be      | de     | no_right_turn @ (Mo-Fr 07:00-11:00) |

        And the relations
            | type        | way:from | node:via | way:to | restriction:conditional             |
            | restriction | be       | e        | de     | no_right_turn @ (Mo-Fr 16:00-18:00) |

        # way restriction is off, node-restriction is on
        When I route I should get
            | from | to | route             | turns                                                               | locations   |
            | a    | d  | ab,be,ef,ef,de,de | depart,turn right,turn left,continue uturn,new name straight,arrive | a,b,e,f,e,d |
            | a    | f  | ab,be,ef,ef       | depart,turn right,turn left,arrive                                  | a,b,e,f     |
            | c    | d  | bc,be,ef,ef,de,de | depart,turn left,turn left,continue uturn,new name straight,arrive  | c,b,e,f,e,d |
            | c    | f  | bc,be,ef,ef       | depart,turn left,turn left,arrive                                   | c,b,e,f     |

    @restriction-way @overlap
    Scenario: Car - prohibit turn
        Given the extract extra arguments "--parse-conditional-restrictions"
        # 5pm Wed 02 May, 2017 GMT
        Given the contract extra arguments "--time-zone-file=test/data/tz/{timezone_names}/guinea.geojson --parse-conditionals-from-now=1493744400"
        Given the customize extra arguments "--time-zone-file=test/data/tz/{timezone_names}/guinea.geojson --parse-conditionals-from-now=1493744400"

        Given the node map
            """
            c
            |
            |   f
            |   |
            b---e
            |   |
            |   d
            |
            a
            """

        And the ways
            | nodes |
            | ab    |
            | bc    |
            | be    |
            | de    |
            | ef    |

        And the relations
            | type        | way:from | way:via | way:to | restriction:conditional             |
            | restriction | ab       | be      | de     | no_right_turn @ (Mo-Fr 16:00-18:00) |

        And the relations
            | type        | way:from | node:via | way:to | restriction:conditional             |
            | restriction | be       | e        | de     | no_right_turn @ (Mo-Fr 07:00-11:00) |

        # node restrictino is off, way restriction is on
        When I route I should get
            | from | to | route             | turns                                                               | locations   |
            | a    | d  | ab,be,ef,ef,de,de | depart,turn right,turn left,continue uturn,new name straight,arrive | a,b,e,f,e,d |
            | a    | f  | ab,be,ef,ef       | depart,turn right,turn left,arrive                                  | a,b,e,f     |
            | c    | d  | bc,be,de,de       | depart,turn left,turn right,arrive                                  | c,b,e,d     |
            | c    | f  | bc,be,ef,ef       | depart,turn left,turn left,arrive                                   | c,b,e,f     |
