@routing @speed @testbot
Feature: Testbot - speeds

    Background: Use specific speeds
        Given the profile "testbot"

    Scenario: Testbot - Speed on roads
        Then routability should be
            | highway   | bothw   |
            | primary   | 36 km/h |
            | unknown   | 24 km/h |
            | secondary | 18 km/h |
            | tertiary  | 12 km/h |

    Scenario: Testbot - Speed on rivers
        Then routability should be
            | highway | forw    | backw   |
            | river   | 36 km/h | 16 km/h |
