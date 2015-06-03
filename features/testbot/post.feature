@post @testbot
Feature: POST request

    Background:
        Given the profile "testbot"

    Scenario: Accept POST Request
        Given the node locations
            | node | lat  | lon  |
            | a    | 1.00 | 1.00 |
            | b    | 1.01 | 1.00 |

        And the ways
            | nodes |
            | ab    |

        When I request post I should get
            | request                     | status_code |
            | locate?loc=1.0,1.0          | 200         |
            | nearest?loc=1.0,1.0         | 200         |
            | viaroute?loc=1,1&loc=1.01,1 | 200         |
            | match?loc=1,1&loc=1.01,1    | 200         |
            | table?loc=1,1&loc=1.01,1    | 200         |
