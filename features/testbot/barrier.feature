@routing @testbot @alternative
Feature: Alternative route

    Background:
        Given the profile "testbot"

        And the node map
            |   | f | g | h |   |
            |   |   | e |   |   |
            | a | b | c | d | z |

        And the nodes
            | node | barrier |
            | b    | gate    |
            | e    | gate    |
            | d    | gate    |

        And the ways
            | nodes |
            | ab    |
            | bc    |
            | cd    |
            | dz    |
            | af    |
            | fg    |
            | ge    |
            | ec    |
            | gh    |
            | hz    |

    Scenario: Route Around

        When I route I should get
            | from | to | route       | turns                                               |
            | a    | z  | af,fg,gh,hz | head,slight_right,straight,slight_right,destination |
            | c    | g  | ec,ge       | head,barrier,destination                            |
