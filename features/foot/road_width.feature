@routing @foot
Feature: Foot road width

    Background:
        Given the profile "foot"

    Scenario: Foot road width uses physical direction on an ordinary one-way road
        Given a grid size of 4 meters
        Given the node map
            """
            a b c d


            1 2 3 4
            e f g h
            """

        And the ways
            | nodes | highway | oneway | width:lanes:forward |
            | abcd  | primary | yes    | 13\|13               |
            | efgh  | primary | no     |                     |

        When I match I should get
            | trace | matchings |
            | 1234  | abcd      |
