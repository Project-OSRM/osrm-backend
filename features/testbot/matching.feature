@match @testbot
Feature: Basic Map Matching

    Background:
        Given the profile "testbot"
        Given a grid size of 10 meters

    Scenario: Testbot - Map matching with outlier that has no candidate
        Given a grid size of 100 meters
        Given the node map
            | a | b | c | d |
            |   |   |   |   |
            |   |   |   |   |
            |   |   |   |   |
            |   |   | 1 |   |

        And the ways
            | nodes | oneway |
            | abcd  | no     |

        When I match I should get
            | trace | timestamps | matchings |
            | ab1d  | 0 1 2 3    | abcd      |

    Scenario: Testbot - Map matching with trace splitting
        Given the node map
            | a | b | c | d |
            |   |   | e |   |

        And the ways
            | nodes | oneway |
            | abcd  | no     |

        When I match I should get
            | trace | timestamps | matchings |
            | abcd  | 0 1 62 63  | ab,cd     |

    Scenario: Testbot - Map matching with core factor
        Given the contract extra arguments "--core 0.8"
        Given the node map
            | a | b | c | d |
            |   |   | e |   |

        And the ways
            | nodes | oneway |
            | abcd  | no     |

        When I match I should get
            | trace | timestamps | matchings |
            | abcd  | 0 1 2 3    | abcd      |

    Scenario: Testbot - Map matching with small distortion
        Given the node map
            | a | b | c | d | e |
            |   | f |   |   |   |
            |   |   |   |   |   |
            |   |   |   |   |   |
            |   |   |   |   |   |
            |   | h |   |   | k |

        # The second way does not need to be a oneway
        # but the grid spacing triggers the uturn
        # detection on f
        And the ways
            | nodes | oneway |
            | abcde | no     |
            | bfhke | yes    |

        When I match I should get
            | trace  | matchings |
            | afcde  | abcde     |

    Scenario: Testbot - Map matching with oneways
        Given a grid size of 10 meters
        Given the node map
            | a | b | c | d |
            | e | f | g | h |

        And the ways
            | nodes | oneway |
            | abcd  | yes    |
            | hgfe  | yes    |

        When I match I should get
            | trace | matchings |
            | dcba  | hgfe      |

    Scenario: Testbot - Matching with oneway streets
        Given a grid size of 10 meters
        Given the node map
            | a | b | c | d |
            | e | f | g | h |

        And the ways
            | nodes | oneway |
            | ab    | yes    |
            | bc    | yes    |
            | cd    | yes    |
            | hg    | yes    |
            | gf    | yes    |
            | fe    | yes    |

        When I match I should get
            | trace | matchings |
            | dcba  | hg,gf,fe  |
            | efgh  | ab,bc,cd  |

    Scenario: Testbot - Duration details
        Given the node map
            | a | b | c | d | e |   | g | h |
            |   |   | i |   |   |   |   |   |

        And the ways
            | nodes    | oneway |
            | abcdegh  | no     |
            | ci       | no     |

        When I match I should get
            | trace | matchings | annotation                                                                     |
            | abeh  | abcedgh   | 1:9.897633,0:0,1:10.008842,1:10.008842,1:10.008842,0:0,2:20.017685,1:10.008842 |
            | abci  | abc,ci    | 1:9.897633,0:0,1:10.008842,0:0.111209,1:10.010367                              |
