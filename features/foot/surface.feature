@routing @foot @surface
Feature: Foot - Surfaces

    Background:
        Given the profile "foot"

    Scenario: Foot - Slow surfaces
        Then routability should be
            | highway | surface     | bothw      |
            | footway |             | 145 s ~10% |
            | footway | fine_gravel | 193 s ~10% |
            | footway | gravel      | 193 s ~10% |
            | footway | pebblestone | 193 s ~10% |
            | footway | mud         | 289 s ~10% |
            | footway | sand        | 289 s ~10% |
