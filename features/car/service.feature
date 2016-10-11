@routing @car @surface
Feature: Car - Surfaces

    Background:
        Given the profile "car"

    @todo
    Scenario: Car - Surface should reduce speed
        Then routability should be
            | highway  | service           | forw       | backw       |
            | service  | alley             | 5 km/h +-1 | 5 km/h +-1  |
            | service  | emergency_access  |            |             |
            | service  | driveway          | 15 km/h +-1| 15 km/h +-1 |

