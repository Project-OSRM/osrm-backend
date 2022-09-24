@routing @foot
Feature: Foot - Intersections

    Background:
        Given the profile "foot"
        Given a grid size of 2 meters

    # https://github.com/Project-OSRM/osrm-backend/issues/6218
    Scenario: Foot - Handles non-planar intersections
        Given the node map

            """
                f
                |
                a
                |
            b---c---d
                |
                e
            """

        And the ways
            | nodes | highway | foot | layer |
            | ac    | footway | yes  | 0     |
            | bc    | footway | yes  | 0     |
            | cd    | footway | yes  | 0     |
            | cef   | footway | yes  | 1     |

        When I route I should get
            | from | to | route    |
            | a    | d  | ac,cd,cd |
