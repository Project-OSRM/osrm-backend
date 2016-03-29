@routing @bicycle @roundabout @instruction
Feature: Roundabout Instructions

    Background:
        Given the profile "bicycle"

    Scenario: Bicycle - Roundabout instructions
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
            | u    | v  | uc,vd | depart,roundabout-exit-1,arrive |
            | u    | s  | uc,sa | depart,roundabout-exit-2,arrive |
            | u    | t  | uc,tb | depart,roundabout-exit-3,arrive |
