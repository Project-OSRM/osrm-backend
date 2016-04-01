@routing @car @roundabout @instruction
Feature: Roundabout Instructions

    Background:
        Given the profile "car"

    Scenario: Car - Roundabout instructions
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
            | from | to | route    | turns                            |
            | s    | t  | sa,tb,tb | depart,roundabout-exit-1,arrive |
            | s    | u  | sa,uc,uc | depart,roundabout-exit-2,arrive |
            | s    | v  | sa,vd,vd | depart,roundabout-exit-3,arrive |
            | u    | v  | uc,vd,vd | depart,roundabout-exit-1,arrive |
            | u    | s  | uc,sa,sa | depart,roundabout-exit-2,arrive |
            | u    | t  | uc,tb,tb | depart,roundabout-exit-3,arrive |
