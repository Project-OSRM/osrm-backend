@routing  @guidance @post-processing
Feature: General Post-Processing related features

    Background:
        Given the profile "car"
        Given a grid size of 0.1 meters

    # this testcase used to crash geometry generation (at that time handled during intersection generation)
    Scenario: Regression Test 2754 
        Given the node map
            """
            a b c d e






























                    f g h i j
            """

        And the ways
            | nodes |
            | abcde |
            | ef    |
            | fghij |

        When I route I should get
            | waypoints | route |
            | a,j       | ef,ef |
