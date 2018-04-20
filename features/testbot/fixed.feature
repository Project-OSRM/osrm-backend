@routing @testbot @fixed
Feature: Fixed bugs, kept to check for regressions

    Background:
        Given the profile "testbot"

    @726
    Scenario: Weird looping, manual input
        Given the node locations
            | node | lat       | lon       |
            | a    | 55.660778 | 12.573909 |
            | b    | 55.660672 | 12.573693 |
            | c    | 55.660128 | 12.572546 |
            | d    | 55.660015 | 12.572476 |
            | e    | 55.660119 | 12.572325 |
            | x    | 55.660818 | 12.574051 |
            | y    | 55.660073 | 12.574067 |

        And the ways
            | nodes |
            | abc   |
            | cdec  |

        When I route I should get
            | from | to | route   |
            | x    | y  | abc,abc |

    Scenario: Step trimming with very short segments
        Given a grid size of 0.1 meters
        Given the node map
            """
            a 1 b c d 2 e
            """

        Given the ways
            | nodes | oneway |
            | ab    | yes    |
            | bcd   | yes    |
            | de    | yes    |

        When I route I should get
            | from | to | route   |
            | 1    | 2  | bcd,bcd |

    #############################
    # This test models the OSM map at the location for
    #    https://github.com/Project-OSRM/osrm-backend/issues/5039
    #############################
    Scenario: Mixed Entry and Exit and segregated
        Given the profile file "car" initialized with
            """
            profile.properties.left_hand_driving = true
            """
            Given the node locations
            | node | lon             | lat             |
            | a    | 171.12889297029 | -42.58425289548 |
            | b    | 171.1299357     | -42.5849295     |
            | c    | 171.1295427     | -42.5849385     |
            | d    | 171.1297356     | -42.5852029     |
            | e    | 171.1296909     | -42.5851986     |
            | f    | 171.1295097     | -42.585007      |
            | g    | 171.1298225     | -42.5851928     |
            | h    | 171.1300262     | -42.5859122     |
            | i    | 171.1292651     | -42.584698      |
            | j    | 171.1297209     | -42.5848569     |
            | k    | 171.1297188     | -42.5854056     |
            | l    | 171.1298326     | -42.5857266     |
            | m    | 171.1298871     | -42.5848922     |
            | n    | 171.1296505     | -42.585189      |
            | o    | 171.1295206     | -42.5850862     |
            | p    | 171.1296106     | -42.5848862     |
            | q    | 171.1299784     | -42.5850191     |
            | r    | 171.1298867     | -42.5851671     |
            | s    | 171.1306955     | -42.5845812     |
            | t    | 171.129525      | -42.584807      |
            | u    | 171.1299705     | -42.584984      |
            | v    | 171.1299067     | -42.5849073     |
            | w    | 171.1302061     | -42.5848173     |
            | x    | 171.12975       | -42.5855753     |
            | y    | 171.129969      | -42.585079      |
            | 1    | 171.131511926651| -42.584306746421966 |
            | 2    | 171.128743886947| -42.58414875714669 |
        And the ways
            | nodes     | highway | maxspeed | name                    | ref   | surface | junction   | oneway |
            | ws        | primary | 100      | Taramakau Highway       | SH 6  | asphalt |            |        |
            | kxlh      | trunk   |          | Otira Highway           | SH 73 |         |            |        |
            | ai        | primary | 100      | Kumara Junction Highway | SH 6  | asphalt |            |        |
            | qyrgdenof | primary | 100      | Kumara Junction         |       |         | roundabout | yes    |
            | ke        | trunk   |          | Otira Highway           | SH 73 |         |            | yes    |
            | itj       | primary | 100      | Kumara Junction Highway | SH 6  |         |            | yes    |
            | gk        | trunk   |          | Otira Highway           | SH 73 |         |            | yes    |
            | fi        | primary | 100      | Kumara Junction Highway | SH 6  |         |            | yes    |
            | wq        | primary | 100      | Taramakau Highway       | SH 6  |         |            | yes    |
            | vw        | primary | 100      | Taramakau Highway       | SH 6  |         |            | yes    |
            | vbuq      | primary | 100      | Kumara Junction         |       |         | roundabout | yes    |
            | jmv       | primary | 100      | Kumara Junction         |       |         | roundabout | yes    |
            | fcpj      | primary | 100      | Kumara Junction         |       |         | roundabout | yes    |

        When I route I should get
            | waypoints | route           | turns                                                    |
            | 1,2       | Taramakau Highway,Kumara Junction Highway,Kumara Junction Highway,Kumara Junction Highway | depart,Kumara Junction-exit-2,exit rotary slight left,arrive |

