@routing @bicycle @names
Feature: Bike - Street names in instructions

    Background:
        Given the profile "bicycle"

    Scenario: Bike - A named street
        Given the node map
            """
            a b
              c
            """

        And the ways
            | nodes | name     | ref |
            | ab    | My Way   | A6  |
            | bc    | Your Way | A7  |

        When I route I should get
            | from | to | route           | ref   |
            | a    | c  | My Way,Your Way | A6,A7 |

    @unnamed
    Scenario: Bike - No longer use way type to describe unnamed ways, see #3231
        Given the node map
            """
            a b c d
            """

        And the ways
            | nodes | highway  | name |
            | ab    | cycleway |      |
            | bcd   | track    |      |

        When I route I should get
            | from | to | route |
            | a    | d  | ,     |
