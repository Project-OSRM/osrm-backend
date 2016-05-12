@testbot @64bit
Feature: Support 64bit node IDs

    # Without 64bit support, this test should fail
    Scenario: 64bit overflow conflicts
        Given the node locations
            | node | lat       | lon       | id         |
            | a    | 55.660778 | 12.573909 | 1          |
            | b    | 55.660672 | 12.573693 | 2          |
            | c    | 55.660128 | 12.572546 | 3          |
            | d    | 55.660015 | 12.572476 | 4294967297 |
            | e    | 55.660119 | 12.572325 | 4294967298 |
            | x    | 55.660818 | 12.574051 | 4294967299 |
            | y    | 55.660073 | 12.574067 | 4294967300 |

        And the ways
            | nodes |
            | abc   |
            | cdec  |

        When I route I should get
            | from | to | route   | turns         |
            | x    | y  | abc,abc | depart,arrive |
