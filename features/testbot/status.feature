@routing @status
Feature: Status messages

    Background:
        Given the profile "testbot"

    Scenario: No route found
        Given the node map
            | a | b |
            |   |   |
            | c | d |

        Given the ways
            | nodes |
            | ab    |
            | cd    |
        
        When I route I should get
            | from | to | route | status | message                          |
            | a    | b  | ab    | 0      | Found route between points       |
            | c    | d  | cd    | 0      | Found route between points       |
            | a    | c  |       | 207    | Cannot find route between points |
            | b    | d  |       | 207    | Cannot find route between points |
