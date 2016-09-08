@routing @guidance
Feature: Features related to bugs

    Background:
        Given the profile "car"
        Given a grid size of 5 meters

    @2852
    Scenario: Loop
        Given the node map
            | a | 1 |   | g |   |   | b |
            |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |
            | e |   |   |   |   |   | f |
            |   |   |   |   |   |   |   |
            |   |   |   |   |   |   | 2 |
            | d |   |   | h |   |   | c |

        And the ways
            | nodes | name   | oneway |
            | agb   | top    | yes    |
            | bfc   | right  | yes    |
            | chd   | bottom | yes    |
            | dea   | left   | yes    |

        And the nodes
            | node | highway         |
            | g    | traffic_signals |
            | f    | traffic_signals |
            | h    | traffic_signals |
            | e    | traffic_signals |

        When I route I should get
            | waypoints | route           | turns                        |
            | 1,2       | top,right,right | depart,new name right,arrive |
