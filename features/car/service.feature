@routing @car @surface
Feature: Car - Surfaces

    Background:
        Given the profile "car"

    Scenario: Car - Ways tagged service should reduce speed
        Then routability should be
            | highway  | service           | forw        | backw        | forw_rate  |
            | service  | alley             | 15 km/h +-1 | 15 km/h +-1  | 2          |
            | service  | emergency_access  |             |              |            |
            | service  | driveway          | 15 km/h +-1 | 15 km/h +-1  | 2          |
            | service  | drive-through     | 15 km/h +-1 | 15 km/h +-1  | 2          |
            | service  | parking           | 15 km/h +-1 | 15 km/h +-1  | 2          |
