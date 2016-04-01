@routing @testbot @via
Feature: Via points

    Background:
        Given the profile "testbot"

        And a grid size of 4 meters

    Scenario: Basic Right Turn
        Given the node map
            | a | b | c | d | e | f | g |
            |   |   |   |   |   | h |   |
            |   |   |   |   |   | i |   |
            |   |   |   |   |   | j |   |
            |   |   |   |   |   | k |   |

        And the ways
            | nodes   | oneway |
            | abcdefg | yes    |
            | ehijk   | yes    |

        When I route I should get
            | from | to | route               | distance  | turns               |
            | a    | k  | abcdefg,ehijk,ehijk |  34m +-1  | depart,right,arrive |

    Scenario: Slight Turn
        Given the node map
            | a | b | c | d | e | f | g |   |
            |   |   |   |   |   | h | i |   |
            |   |   |   |   |   |   |   | j |
            |   |   |   |   |   |   |   | k |

        And the ways
            | nodes   | oneway |
            | abcdefg | yes    |
            | ehijk   | yes    |

        When I route I should get
            | from | to | route               | distance  | turns                      |
            | a    | k  | abcdefg,ehijk,ehijk |  35m +-1  | depart,slight right,arrive |

    Scenario: Nearly Slight Turn
        Given the node map
            | a | b | c | d | e | f | g |   |
            |   |   |   |   |   | h |   |   |
            |   |   |   |   |   |   | i |   |
            |   |   |   |   |   |   |   | j |
            |   |   |   |   |   |   |   | k |

        And the ways
            | nodes   | oneway |
            | abcdefg | yes    |
            | ehijk   | yes    |

        When I route I should get
            | from | to | route               | distance  | turns                      |
            | a    | k  | abcdefg,ehijk,ehijk |  37m +-1  | depart,right,arrive        |

    Scenario: Nearly Slight Turn (Variation)
        Given the node map
            | a | b | c | d | e | f | g |   |
            |   |   |   |   |   | h |   |   |
            |   |   |   |   |   |   | i |   |
            |   |   |   |   |   |   | j |   |
            |   |   |   |   |   |   |   | k |

        And the ways
            | nodes   | oneway |
            | abcdefg | yes    |
            | ehijk   | yes    |

        When I route I should get
            | from | to | route               | distance  | turns                      |
            | a    | k  | abcdefg,ehijk,ehijk |  37m +-1  | depart,right,arrive        |
