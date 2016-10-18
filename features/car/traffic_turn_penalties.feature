@routing @speed @traffic
Feature: Traffic - turn penalties

    Background: Evenly spaced grid with multiple intersections
        Given the node map
            """
              a   b
            c d e f g
              h   i
            j k l m n
              o   p
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
            | h    | 8  |
            | i    | 9  |
            | j    | 10 |
            | k    | 11 |
            | l    | 12 |
            | m    | 13 |
            | n    | 14 |
            | o    | 15 |
            | p    | 16 |

        And the ways
            | nodes | highway |
            | ad    | primary |
            | cd    | primary |
            | def   | primary |
            | dhk   | primary |

            | bf    | primary |
            | fg    | primary |
            | fim   | primary |

            | jk    | primary |
            | klm   | primary |
            | ko    | primary |

            | mn    | primary |
            | mp    | primary |
        And the profile "car"
        And the extract extra arguments "--generate-edge-lookup"

    Scenario: Weighting not based on turn penalty file
        When I route I should get
            | from | to | route           | speed   | time      |
            | a    | h  | ad,dhk,dhk      | 52 km/h | 14s +-1   |
                                                                  # straight
            | i    | g  | fim,fg,fg       | 45 km/h | 16s +-1   |
                                                                  # right
            | a    | e  | ad,def,def      | 38 km/h | 19s +-1   |
                                                                  # left
            | c    | g  | cd,def,fg,fg    | 52 km/h | 27s +-1   |
                                                                  # double straight
            | p    | g  | mp,fim,fg,fg    | 48 km/h | 29s +-1   |
                                                                  # straight-right
            | a    | l  | ad,dhk,klm,klm  | 44 km/h | 33s +-1   |
                                                                  # straight-left
            | l    | e  | klm,dhk,def,def | 45 km/h | 32s +-1   |
                                                                  # double right
            | g    | n  | fg,fim,mn,mn    | 38 km/h | 38s +-1   |
                                                                  # double left

    Scenario: Weighting based on turn penalty file
        Given the turn penalty file
            """
            9,6,7,1.8
            9,13,14,24.5
            8,4,3,35
            12,11,8,9
            8,11,12,23
            1,4,5,-0.2
            """
        And the contract extra arguments "--turn-penalty-file {penalties_file}"
        When I route I should get
            | from | to | route                 | speed   | time      |
            | a    | h  | ad,dhk,dhk            | 52 km/h | 14s +-1   |
                                                                              # straight
            | i    | g  | fim,fg,fg             | 46 km/h | 15s +-1   |
                                                                              # right - ifg penalty
            | a    | e  | ad,def,def            | 53 km/h | 14s +-1   |
                                                                              # left - faster because of negative ade penalty
            | c    | g  | cd,def,fg,fg          | 52 km/h | 27s +-1   |
                                                                              # double straight
            | p    | g  | mp,fim,fg,fg          | 49 km/h | 29s +-1   |
                                                                              # straight-right - ifg penalty
            | a    | l  | ad,def,fim,klm,klm    | 48 km/h | 45s +-1   |
                                                                              # was straight-left - forced around by hkl penalty
            | l    | e  | klm,fim,def,def       | 38 km/h | 38s +-1   |
                                                                              # double right - forced left by lkh penalty
            | g    | n  | fg,fim,mn,mn          | 25 km/h | 57s +-1   |
                                                                              # double left - imn penalty
            | j    | c  | jk,klm,fim,def,cd,cd  | 44 km/h | 65.8s +-1 |
                                                                              # double left - hdc penalty ever so slightly higher than imn; forces all the way around

    Scenario: Too-negative penalty clamps, but does not fail
        Given the contract extra arguments "--turn-penalty-file {penalties_file}"
        And the profile "testbot"
        And the turn penalty file
            """
            1,4,5,-10
            """
        When I route I should get
            | from | to | route      | time    |
            | a    | d  | ad,ad      | 10s +-1 |
            | a    | e  | ad,def,def | 10s +-1 |
            | b    | f  | bf,bf      | 10s +-1 |
            | b    | g  | bf,fg,fg   | 20s +-1 |
