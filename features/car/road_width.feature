@routing @car
Feature: Car road width

    Background:
        Given the profile "car"

    Scenario: Car road width uses physical direction rather than motor vehicle one-way access
        Given a grid size of 4 meters
        Given the node map
            """
            a b c d


            1 2 3 4
            e f g h
            """

        And the ways
            | nodes | highway | oneway | oneway:motor_vehicle | width:forward | width:backward |
            | abcd  | primary | no     | yes                  | 13            | 13             |
            | efgh  | primary | no     |                      |               |                |

        When I match I should get
            | trace | matchings |
            | 1234  | abcd      |
