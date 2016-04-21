@routing @speed @traffic
Feature: Traffic - turn penalties

    Background: Use specific speeds
        And the node map
            |   | a |   | b |   |
            | c | d | e | f | g |
            |   | h |   | i |   |
            | j | k | l | m | n |
            |   | o |   | p |   |
        And the ways
            | nodes | highway |
            | ad    | primary |
            | cd    | primary |
            | de    | primary |
            | dh    | primary |

            | bf    | primary |
            | ef    | primary |
            | fg    | primary |
            | fi    | primary |

            | hk    | primary |
            | jk    | primary |
            | kl    | primary |
            | ko    | primary |

            | im    | primary |
            | lm    | primary |
            | mn    | primary |
            | mp    | primary |
        And the profile "car"
        And the extract extra arguments "--generate-edge-lookup"

    Scenario: Weighting not based on turn penalty file
        When I route I should get
            | from | to | route          | speed   | time      |
            | a    | h  | ad,dh,dh       | 63 km/h | 11.5s +-1 |
                                                                  # straight
            | i    | g  | fi,fg,fg       | 59 km/h | 12s  +-1  |
                                                                  # right
            | a    | e  | ad,de,de       | 57 km/h | 12.5s +-1 |
                                                                  # left
            | c    | g  | cd,de,ef,fg,fg | 63 km/h | 23s +-1   |
                                                                  # double straight
            | p    | g  | mp,im,fi,fg,fg | 61 km/h | 23.5s +-1 |
                                                                  # straight-right
            | a    | l  | ad,dh,hk,kl,kl | 60 km/h | 24s +-1   |
                                                                  # straight-left
            | l    | e  | kl,hk,dh,de,de | 59 km/h | 24.5s +-1 |
                                                                  # double right
            | g    | n  | fg,fi,im,mn,mn | 57 km/h | 25s +-1   |
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
            | from | to | route                      | speed   | time      |
            | a    | h  | ad,dh,dh                   | 63 km/h | 11.5s +-1 |
                                                                              # straight
            | i    | g  | fi,fg,fg                   | 55 km/h | 13s +-1   |
                                                                              # right - ifg penalty
            | a    | e  | ad,de,de                   | 64 km/h | 11s +-1   |
                                                                              # left - faster because of negative ade penalty
            | c    | g  | cd,de,ef,fg,fg             | 63 km/h | 23s +-1   |
                                                                              # double straight
            | p    | g  | mp,im,fi,fg,fg             | 59 km/h | 24.5s +-1 |
                                                                              # straight-right - ifg penalty
            | a    | l  | ad,de,ef,fi,im,lm,lm       | 61 km/h | 35.5s +-1 |
                                                                              # was straight-left - forced around by hkl penalty
            | l    | e  | lm,im,fi,ef,ef             | 57 km/h | 25s +-1   |
                                                                              # double right - forced left by lkh penalty
            | g    | n  | fg,fi,im,mn,mn             | 30 km/h | 47.5s +-1   |
                                                                              # double left - imn penalty
            | j    | c  | jk,kl,lm,im,fi,ef,de,cd,cd | 60 km/h | 48s +-1   |
                                                                              # double left - hdc penalty ever so slightly higher than imn; forces all the way around

    Scenario: Too-negative penalty triggers assert (negative edge weight)
        Given the contract extra arguments "--turn-penalty-flie penalties.csv"
        And the turn penalty file
            """
            1,4,5,-9.3
            """
        And the data has been extracted
        When I run "osrm-contract --turn-penalty-file penalties.csv {extracted_base}.osrm"
        Then it should exit with code 1
        And stderr should contain "[warn] [exception]"
