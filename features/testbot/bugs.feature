@routing @testbot @bug
Feature: Known bugs

    Background:
        Given the profile "testbot"

    Scenario: Routing on a oneway roundabout
        Given the node map
            |   | d | c |   |
            | e |   |   | b |
            | f |   |   | a |
            |   | g | h |   |

        And the ways
            | nodes | oneway |
            | ab    | yes    |
            | bc    | yes    |
            | cd    | yes    |
            | de    | yes    |
            | ef    | yes    |
            | fg    | yes    |
            | gh    | yes    |
            | ha    | yes    |

        When I route I should get
            | from | to | route                |
            | a    | b  | ab                   |
            | b    | c  | bc                   |
            | c    | d  | cd                   |
            | d    | e  | de                   |
            | e    | f  | ef                   |
            | f    | g  | fg                   |
            | g    | h  | gh                   |
            | h    | a  | ha                   |
            | b    | a  | bc,cd,de,ef,fg,gh,ha |
            | c    | b  | cd,de,ef,fg,gh,ha,ab |
            | d    | c  | de,ef,fg,gh,ha,ab,bc |
            | e    | d  | ef,fg,gh,ha,ab,bc,cd |
            | f    | e  | fg,gh,ha,ab,bc,cd,de |
            | g    | f  | gh,ha,ab,bc,cd,de,ef |
            | h    | g  | ha,ab,bc,cd,de,ef,fg |
            | a    | h  | ab,bc,cd,de,ef,fg,gh |

    @726
    Scenario: Weird looping, manual input
        Given the node locations
            | node | lat       | lon       |
            | a    | 55.660778 | 12.573909 |
            | b    | 55.660672 | 12.573693 |
            | c    | 55.660128 | 12.572546 |
            | d    | 55.660015 | 12.572476 |
            | e    | 55.660119 | 12.572325 |
            | x    | 55.660818 | 12.574051 |
            | y    | 55.660073 | 12.574067 |

        And the ways
            | nodes |
            | abc   |
            | cdec  |

        When I route I should get
            | from | to | route | turns            |
            | x    | y  | abc   | head,destination |
