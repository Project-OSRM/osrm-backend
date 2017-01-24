@routing @foot @maxspeed
Feature: Foot - Ignore max speed restrictions

Background: Use specific speeds
    Given the profile "foot"

    Scenario: Foot - Ignore maxspeed
        Then routability should be
            | highway     | maxspeed  | bothw      |
            | residential |           | 145 s ~10% |
            | residential | 1         | 145 s ~10% |
            | residential | 100       | 145 s ~10% |
            | residential | 1         | 145 s ~10% |
            | residential | 1mph      | 145 s ~10% |
            | residential | 1 mph     | 145 s ~10% |
            | residential | 1unknown  | 145 s ~10% |
            | residential | 1 unknown | 145 s ~10% |
            | residential | none      | 145 s ~10% |
            | residential | signals   | 145 s ~10% |
