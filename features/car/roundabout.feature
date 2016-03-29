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
            | from | to | route | turns                            |
            | s    | t  | sa,tb | depart,enter_roundabout-1,arrive |
            | s    | u  | sa,uc | depart,enter_roundabout-2,arrive |
            | s    | v  | sa,vd | depart,enter_roundabout-3,arrive |
            | u    | v  | uc,vd | depart,enter_roundabout-1,arrive |
            | u    | s  | uc,sa | depart,enter_roundabout-2,arrive |
            | u    | t  | uc,tb | depart,enter_roundabout-3,arrive |
