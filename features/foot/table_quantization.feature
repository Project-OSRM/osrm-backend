@routing @foot @area @matrix
Feature: Table searches preserve short road travel next to an open area

    Background:
        Given the profile "foot_area"
        And the node locations
            | node | lon       | lat      |
            | a    | 28.91     | 41       |
            | b    | 28.9101   | 41       |
            | c    | 28.9101   | 41.0001  |
            | d    | 28.91     | 41.0001  |
            | e    | 28.9099   | 41       |
            | f    | 28.9102   | 41       |
            | g    | 28.9099   | 41.0001  |
            | h    | 28.9102   | 41.0001  |
            | 1    | 28.909999 | 41       |
            | 2    | 28.91004  | 41.00005 |
            | 3    | 28.909999 | 41       |
        And the ways
            | nodes | highway    | area |
            | abcda | pedestrian | yes  |
            | ea    | footway    |      |
            | bf    | footway    |      |
            | gd    | footway    |      |
            | ch    | footway    |      |

    # 1 is just outside the plaza. Going through gate a adds the short road
    # segment to the walk across the plaza, in either direction.
    Scenario: An approach walk cannot hide backwards travel on the road
        When I request a travel distance matrix I should get
            |   | 2   | 1   |
            | 2 | 0   | 6.6 |
            | 1 | 6.6 | 0   |
        When I request a travel distance matrix I should get
            |   | 1   | 2   |
            | 1 | 0   | 6.6 |
            | 2 | 6.6 | 0   |
        When I request a travel distance matrix I should get
            |   | 1 | 2   |
            | 1 | 0 | 6.6 |
        When I request a travel distance matrix I should get
            |   | 1   |
            | 1 | 0   |
            | 2 | 6.6 |
        When I request a travel distance matrix I should get
            |   | 1   | 2   |
            | 1 | 0   | 6.6 |
            | 2 | 6.6 | 0   |
            | 3 | 0   | 6.6 |
