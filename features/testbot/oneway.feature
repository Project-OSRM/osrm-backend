@routing @testbot @oneway
Feature: Testbot - oneways

    Background:
        Given the profile "testbot"
        Given a grid size of 250 meters

    Scenario: Testbot - Simple oneway
        Then routability should be
            | highway | foot | oneway | forw | backw |
            | primary | no   | yes    | x    |       |

    Scenario: Simple reverse oneway
        Then routability should be
            | highway | foot | oneway | forw | backw |
            | primary | no   | -1     |      | x     |

    Scenario: Testbot - Handle various oneway tag values
        Then routability should be
            | foot | oneway   | forw | backw |
            | no   |          | x    | x     |
            | no   | nonsense | x    | x     |
            | no   | no       | x    | x     |
            | no   | false    | x    | x     |
            | no   | 0        | x    | x     |
            | no   | yes      | x    |       |
            | no   | true     | x    |       |
            | no   | 1        | x    |       |
            | no   | -1       |      | x     |
