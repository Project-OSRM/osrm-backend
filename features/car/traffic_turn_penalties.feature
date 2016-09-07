@routing @speed @traffic
Feature: Traffic - turn penalties

    Background: Evenly spaced grid with multiple intersections
        Given the node map
            |      | a:1  |      | b:2  |      |
            | c:3  | d:4  | e:5  | f:6  | g:7  |
            |      | h:8  |      | i:9  |      |
            | j:10 | k:11 | l:12 | m:13 | n:14 |
            |      | o:15 |      | p:16 |      |
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
            | a    | h  | ad,dhk,dhk      | 63 km/h | 11.5s +-1 |
                                                                  # straight
            | i    | g  | fim,fg,fg       | 53 km/h | 13.5s +-1 |
                                                                  # right
            | a    | e  | ad,def,def      | 43 km/h | 16.7s +-1 |
                                                                  # left
            | c    | g  | cd,def,fg,fg    | 63 km/h | 23s +-1   |
                                                                  # double straight
            | p    | g  | mp,fim,fg,fg    | 58 km/h | 24.9s +-1 |
                                                                  # straight-right
            | a    | l  | ad,dhk,klm,klm  | 51 km/h | 28.1s +-1 |
                                                                  # straight-left
            | l    | e  | klm,dhk,def,def | 53 km/h | 27s +-1   |
                                                                  # double right
            | g    | n  | fg,fim,mn,mn    | 43 km/h | 33.4s +-1 |
                                                                  # double left

    Scenario: Weighting based on turn penalty file
        Given the turn penalty file
            """
            9,6,7,1.8
            9,13,14,24.5
            8,4,3,30
            12,11,8,9
            8,11,12,23
            1,4,5,-0.2
            """
        And the contract extra arguments "--turn-penalty-file penalties.csv"
        When I route I should get
            | from | to | route                 | speed   | time      |
            | a    | h  | ad,dhk,dhk            | 63 km/h | 11.5s +-1 |
                                                                              # straight
            | i    | g  | fim,fg,fg             | 55 km/h | 13s +-1   |
                                                                              # right - ifg penalty
            | a    | e  | ad,def,def            | 64 km/h | 11s +-1   |
                                                                              # left - faster because of negative ade penalty
            | c    | g  | cd,def,fg,fg          | 63 km/h | 23s +-1   |
                                                                              # double straight
            | p    | g  | mp,fim,fg,fg          | 59 km/h | 24.5s +-1 |
                                                                              # straight-right - ifg penalty
            | a    | l  | ad,def,fim,klm,klm    | 57 km/h | 38.2s +-1 |
                                                                              # was straight-left - forced around by hkl penalty
            | l    | e  | klm,fim,def,def       | 43 km/h | 33.4s +-1 |
                                                                              # double right - forced left by lkh penalty
            | g    | n  | fg,fim,mn,mn          | 27 km/h | 52.6s +-1 |
                                                                              # double left - imn penalty
            | j    | c  | jk,klm,fim,def,cd,cd  | 51 km/h | 56.2s +-1 |
                                                                              # double left - hdc penalty ever so slightly higher than imn; forces all the way around

    Scenario: Too-negative penalty clamps, but does not fail
        Given the contract extra arguments "--turn-penalty-file penalties.csv"
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
