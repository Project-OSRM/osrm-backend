@routing  @guidance @post-processing
Feature: General Post-Processing related features

    Background:
        Given the profile "car"
        Given a grid size of 10 meters

    # this testcase used to crash geometry generation (at that time handled during intersection generation)
    Scenario: Regression test #2424
        Given the node map
            """
                e           i
            a   b   c   d   h   k   m
                    f           l
            """

        And the ways
            | nodes  | name                 |
            | abcd   | Fritz-Elsas-Straße   |
            | hkm    | Fritz-Elsas-Straße   |
            | dhi    | Martin-Luther-Straße |
            | be     | corner               |
            | kl     | corner               |
            | cf     | corner               |
