@routing @pbf_output @testbot
Feature: Outputting protobuffer format

    Background:
        Given the profile "testbot"
        And the query options
            | output | pbf |
    
    Scenario: Testbot - Protobuffer import, nodes and ways
        Given the node map
            | a | b |

        And the ways
            | nodes |
            | ab    |

        When I route I should get
            | from | to | route |
            | a    | b  | ab    |

