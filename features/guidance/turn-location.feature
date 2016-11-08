@routing  @guidance
Feature: Turn Location Feature

    Background:
        Given the profile "car"
        Given a grid size of 10 meters

    Scenario: Simple feature to test turn locations
        Given the node map
            """
              c
            a b d
            """

        And the ways
            | nodes  | highway |
            | ab     | primary |
            | cb     | primary |
            | db     | primary |

       When I route I should get
            | waypoints | route    | turns                   | locations |
            | a,c       | ab,cb,cb | depart,turn left,arrive | a,b,c     |
