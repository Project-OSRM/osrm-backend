@routing @testbot @roundabout @instruction
Feature: Roundabout Instructions

    Background:
        Given the profile "testbot"

    Scenario: Testbot - Roundabout
        Given the node map
            |   |   | v |   |   |
            |   |   | d |   |   |
            | s | a |   | c | u |
            |   |   | b |   |   |
            |   |   | t |   |   |

        And the ways
            | nodes | junction   |
            | sa    |            |
            | tb    |            |
            | uc    |            |
            | vd    |            |
            | abcda | roundabout |

        When I route I should get
            | from | to | route | turns                            |
            | s    | t  | sa,tb | depart,roundabout-exit-1,arrive |
            | s    | u  | sa,uc | depart,roundabout-exit-2,arrive |
            | s    | v  | sa,vd | depart,roundabout-exit-3,arrive |
            | t    | u  | tb,uc | depart,roundabout-exit-1,arrive |
            | t    | v  | tb,vd | depart,roundabout-exit-2,arrive |
            | t    | s  | tb,sa | depart,roundabout-exit-3,arrive |
            | u    | v  | uc,vd | depart,roundabout-exit-1,arrive |
            | u    | s  | uc,sa | depart,roundabout-exit-2,arrive |
            | u    | t  | uc,tb | depart,roundabout-exit-3,arrive |
            | v    | s  | vd,sa | depart,roundabout-exit-1,arrive |
            | v    | t  | vd,tb | depart,roundabout-exit-2,arrive |
            | v    | u  | vd,uc | depart,roundabout-exit-3,arrive |

    Scenario: Testbot - Roundabout with oneway links
        Given the node map
            |   |   | p | o |   |   |
            |   |   | h | g |   |   |
            | i | a |   |   | f | n |
            | j | b |   |   | e | m |
            |   |   | c | d |   |   |
            |   |   | k | l |   |   |

        And the ways
            | nodes     | junction   | oneway |
            | ai        |            | yes    |
            | jb        |            | yes    |
            | ck        |            | yes    |
            | ld        |            | yes    |
            | em        |            | yes    |
            | nf        |            | yes    |
            | go        |            | yes    |
            | ph        |            | yes    |
            | abcdefgha | roundabout |        |

        When I route I should get
            | from | to | route | turns                            |
            | j    | k  | jb,ck | depart,roundabout-exit-1,arrive |
            | j    | m  | jb,em | depart,roundabout-exit-2,arrive |
            | j    | o  | jb,go | depart,roundabout-exit-3,arrive |
            | j    | i  | jb,ai | depart,roundabout-exit-4,arrive |
            | l    | m  | ld,em | depart,roundabout-exit-1,arrive |
            | l    | o  | ld,go | depart,roundabout-exit-2,arrive |
            | l    | i  | ld,ai | depart,roundabout-exit-3,arrive |
            | l    | k  | ld,ck | depart,roundabout-exit-4,arrive |
            | n    | o  | nf,go | depart,roundabout-exit-1,arrive |
            | n    | i  | nf,ai | depart,roundabout-exit-2,arrive |
            | n    | k  | nf,ck | depart,roundabout-exit-3,arrive |
            | n    | m  | nf,em | depart,roundabout-exit-4,arrive |
            | p    | i  | ph,ai | depart,roundabout-exit-1,arrive |
            | p    | k  | ph,ck | depart,roundabout-exit-2,arrive |
            | p    | m  | ph,em | depart,roundabout-exit-3,arrive |
            | p    | o  | ph,go | depart,roundabout-exit-4,arrive |
