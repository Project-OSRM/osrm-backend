@routing @priority @car
Feature: Car - Priority tag handling
The priority tag indicates traffic right-of-way direction on a way.
priority=forward means forward traffic has right-of-way, penalizing backward.
priority=backward means backward traffic has right-of-way, penalizing forward.

    Background: Use specific speeds
        Given the profile "car"
        Given a grid size of 1000 meters

    Scenario: Car - Priority tag applies directional penalty
        Then routability should be
            | highway     | maxspeed | priority | forw    | backw   | forw_rate | backw_rate |
            | primary     | 60       |          | 48 km/h | 48 km/h | 13.3      | 13.3       |
            | primary     | 60       | forward  | 48 km/h | 48 km/h | 13.3      | 9.3        |
            | primary     | 60       | backward | 48 km/h | 48 km/h | 9.3       | 13.3       |

    Scenario: Car - Priority penalty applies to the permitted direction on oneways when it lacks priority
        Then routability should be
            | highway     | maxspeed | priority | oneway | forw    | backw   | forw_rate | backw_rate |
            | primary     | 60       | forward  | yes    | 48 km/h |         | 13.3      |            |
            | primary     | 60       | backward | yes    | 48 km/h |         | 9.3       |            |
            | primary     | 60       | forward  | -1     |         | 48 km/h |           | 9.3        |
            | primary     | 60       | backward | -1     |         | 48 km/h |           | 13.3       |

    Scenario: Car - Directional penalty avoids bidirectional weight
        Then routability should be
            | highway     | maxspeed | priority | forw    | backw   | forw_rate | backw_rate |
            | primary     | 60       | forward  | 48 km/h | 48 km/h | 13.3      | 9.3        |
            | primary     | 60       | backward | 48 km/h | 48 km/h | 9.3       | 13.3       |
