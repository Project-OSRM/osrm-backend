@routing @car @surface
Feature: Car - Surfaces

    Background:
        Given the profile "car.lua"

    Scenario: Car - Surface should reduce speed
        Then routability should be
            | highway  | service           | forw       | backw       |
            | service  | alley             | 5 km/h +-1 | 5 km/h +-1  |
            | service  | emergency_access  |            |             |
            | service  | driveway          | 5 km/h +-1 | 5 km/h +-1  |
            | service  | drive-through     | 5 km/h +-1 | 5 km/h +-1  |
            | service  | parking           | 5 km/h +-1 | 5 km/h +-1  |
