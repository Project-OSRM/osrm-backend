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

    Scenario: Weighting not based on turn penalty file
        When I route I should get
            | from | to | route           | speed   | weight    | time      |
            | a    | h  | ad,dhk          | 65 km/h | 11s +-1   | 11s +-1   |
                                                                                # straight
            | i    | g  | fim,fg,fg       | 55 km/h | 13s +-1   | 13s +-1   |
                                                                                # right
            | a    | e  | ad,def,def      | 44 km/h | 16.3s +-1 | 16.3s +-1 |
                                                                                # left
            | c    | g  | cd,def,fg       | 65 km/h | 22s +-1   | 22s +-1   |
                                                                                # double straight
            | p    | g  | mp,fim,fg,fg    | 60 km/h | 24s +-1   | 24s +-1   |
                                                                                # straight-right
            | a    | l  | ad,dhk,klm,klm  | 53 km/h | 27s +-1   | 27s +-1   |
                                                                                # straight-left
            | l    | e  | klm,dhk,def,def | 55 km/h | 26s +-1   | 26s +-1   |
                                                                                # double right
            | g    | n  | fg,fim,mn,mn    | 44 km/h | 32s +-1   | 32s +-1   |
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
            # ifg right turn
            # imn left turn
            # hdc left turn
            # lkh right turn
            # hkl left turn
            # ade left turn
        And the contract extra arguments "--turn-penalty-file {penalties_file}"
        And the customize extra arguments "--turn-penalty-file {penalties_file}"
        When I route I should get
            | from | to | route                 | speed   | weight  | time      |
            | a    | h  | ad,dhk                | 65 km/h | 11      | 11s +-1   |
                                                                                # straight
            | i    | g  | fim,fg,fg             | 56 km/h | 12.8    | 12s +-1   |
                                                                                # right - ifg penalty
            | a    | e  | ad,def,def            | 67 km/h | 10.8    | 10s +-1   |
                                                                                # left - faster because of negative ade penalty
            | c    | g  | cd,def,fg             | 65 km/h | 22      | 22s +-1   |
                                                                                # double straight
            | p    | g  | mp,fim,fg,fg          | 61 km/h | 23.8    | 23s +-1   |
                                                                                # straight-right - ifg penalty
            | a    | l  | ad,def,fim,klm,klm    | 58 km/h | 37      | 37s +-1   |
                                                                                # was straight-left - forced around by hkl penalty
            | l    | e  | klm,fim,def,def       | 44 km/h | 32.6    | 32s +-1   |
                                                                                # double right - forced left by lkh penalty
            | g    | n  | fg,fim,mn,mn          | 28 km/h | 51.8    | 51s +-1   |
                                                                                 # double left - imn penalty
            | j    | c  | jk,klm,fim,def,cd     | 53 km/h | 54.6    | 54s +-1   |
                                                                                  # double left - hdc penalty ever so slightly higher than imn; forces all the way around

    Scenario: Too-negative penalty clamps, but does not fail
        Given the profile "testbot"
        And the contract extra arguments "--turn-penalty-file {penalties_file}"
        And the customize extra arguments "--turn-penalty-file {penalties_file}"
        And the turn penalty file
            """
            1,4,5,-10
            """
        When I route I should get
            | from | to | route      | time    |
            # The target point `d` can be in `ad`, `cd`, `deh` and `dhk`
            # The test must be fixed by #2287
            #| a    | d  | ad,ad      | 10s +-1 |
            | a    | e  | ad,def,def | 10s +-1 |
            | b    | f  | bf,bf      | 10s +-1 |
            | b    | g  | bf,fg,fg   | 20s +-1 |
