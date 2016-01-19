@routing @testbot @parallel_arcs
Feature: Oneway Roundabout

    Background:
        Given the profile "testbot"

    Scenario: Simple Circle
        Given the node map
            |   |   | g |   |   |
            | a | b |   | d | f |
            |   |   | c |   |   |

        And the ways
            | nodes | oneway |
            | ab    | no     |
            | bcd   | yes    |
            | df    | no     |
            | dgb   | yes    |

        When I route I should get
            | waypoints | route        |
            | a,f       | ab,bcd,df,df |
            | f,a       | df,dgb,ab,ab |
            | b,d       | bcd,bcd      |
            | d,b       | dgb,dgb      |

    Scenario: Parallel Geometry
        Given the node map
            | a | b | c | g | d | f |

        And the ways
            | nodes | oneway |
            | ab    | no     |
            | bcd   | yes    |
            | df    | no     |
            | dgb   | yes    |

        When I route I should get
            | waypoints | route        |
            | a,f       | ab,bcd,df,df |
            | f,a       | df,dgb,ab,ab |
            | b,d       | bcd,bcd      |
            | d,b       | dgb,dgb      |

     Scenario: Parallel Geometry
        Given the node map
            | a | b |

        And the ways
            | nodes | oneway | highway |
            | ab    | yes    | primary |
            | ba    | yes    | tertiary |

        When I route I should get
            | waypoints | route   | time |
            | a,b       | ab,ab   | 10s  |
            | b,a       | ba,ba   | 30s  |

     Scenario: Parallel Geometry
        Given the node map
            | a | b |

        And the ways
            | nodes | oneway | highway  |
            | ba    | yes    | tertiary |
            | ab    | yes    | primary  |

        When I route I should get
            | waypoints | route   | time |
            | a,b       | ab,ab   | 10s  |
            | b,a       | ba,ba   | 30s  |

     Scenario: Parallel Geometry Contractable
        Given the node map
            | a | b | c | d | e | f | g |

        And the ways
            | nodes | oneway | highway  |
            | gf    | yes    | tertiary |
            | fe    | yes    | tertiary |
            | ed    | yes    | tertiary |
            | dc    | yes    | tertiary |
            | cb    | yes    | tertiary |
            | ba    | yes    | tertiary |
            | ab    | yes    | primary  |
            | bc    | yes    | primary  |
            | cd    | yes    | primary  |
            | de    | yes    | primary  |
            | ef    | yes    | primary  |
            | fg    | yes    | primary  |

        When I route I should get
            | waypoints | route                | time |
            | a,b       | ab,ab                | 10s  |
            | a,g       | ab,bc,cd,de,ef,fg,fg | 60s  |
            | b,a       | ba,ba                | 30s  |
            | g,a       | gf,fe,ed,dc,cb,ba,ba | 180s |
            | b,f       | bc,cd,de,ef,ef       | 40s  |
            | f,b       | fe,ed,dc,cb,cb       | 120s |
