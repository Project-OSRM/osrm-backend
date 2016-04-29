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
            | de    | primary |
            | dhk   | primary |

            | bf    | primary |
            | ef    | primary |
            | fg    | primary |
            | fim   | primary |

            | jk    | primary |
            | kl    | primary |
            | ko    | primary |

            | lm    | primary |
            | mn    | primary |
            | mp    | primary |
        And the profile "car"
        And the extract extra arguments "--generate-edge-lookup"

    Scenario: Weighting not based on turn penalty file
        When I route I should get
            | from | to | route          | speed   | time      |
            | a    | h  | ad,dhk,dhk     | 63 km/h | 11.5s +-1 |
                                                                  # straight
            | i    | g  | fim,fg,fg      | 59 km/h | 12s  +-1  |
                                                                  # right
            | a    | e  | ad,de,de       | 57 km/h | 12.5s +-1 |
                                                                  # left
            | c    | g  | cd,de,ef,fg,fg | 63 km/h | 23s +-1   |
                                                                  # double straight
            | p    | g  | mp,fim,fg,fg   | 61 km/h | 23.5s +-1 |
                                                                  # straight-right
            | a    | l  | ad,dhk,kl,kl   | 60 km/h | 24s +-1   |
                                                                  # straight-left
            | l    | e  | kl,dhk,de,de   | 59 km/h | 24.5s +-1 |
                                                                  # double right
            | g    | n  | fg,fim,mn,mn   | 57 km/h | 25s +-1   |
                                                                  # double left

    Scenario: Weighting based on turn penalty file
        Given the turn penalty file
            """
            9,6,7,1.8
            9,13,14,24.5
            8,4,3,26
            12,11,8,9
            8,11,12,13
            1,4,5,-0.2
            """
        And the contract extra arguments "--turn-penalty-file penalties.csv"
        When I route I should get
            | from | to | route                    | speed   | time      |
            | a    | h  | ad,dhk,dhk               | 63 km/h | 11.5s +-1 |
                                                                              # straight
            | i    | g  | fim,fg,fg                | 55 km/h | 13s +-1   |
                                                                              # right - ifg penalty
            | a    | e  | ad,de,de                 | 64 km/h | 11s +-1   |
                                                                              # left - faster because of negative ade penalty
            | c    | g  | cd,de,ef,fg,fg           | 63 km/h | 23s +-1   |
                                                                              # double straight
            | p    | g  | mp,fim,fg,fg             | 59 km/h | 24.5s +-1 |
                                                                              # straight-right - ifg penalty
            | a    | l  | ad,de,ef,fim,lm,lm       | 61 km/h | 35.5s +-1 |
                                                                              # was straight-left - forced around by hkl penalty
            | l    | e  | lm,fim,ef,ef             | 57 km/h | 25s +-1   |
                                                                              # double right - forced left by lkh penalty
            | g    | n  | fg,fim,mn,mn             | 30 km/h | 47.5s +-1   |
                                                                              # double left - imn penalty
            | j    | c  | jk,kl,lm,fim,ef,de,cd,cd | 60 km/h | 48s +-1   |
                                                                              # double left - hdc penalty ever so slightly higher than imn; forces all the way around

    Scenario: Too-negative penalty clamps, but does not fail
        Given the contract extra arguments "--turn-penalty-file penalties.csv"
        And the profile "testbot"
        And the turn penalty file
            """
            1,4,5,-10
            """
        When I route I should get
            | from | to | route    | time    |
            | a    | d  | ad,ad    | 10s +-1 |
            | a    | e  | ad,de,de | 10s +-1 |
            | b    | f  | bf,bf    | 10s +-1 |
            | b    | g  | bf,fg,fg | 20s +-1 |
