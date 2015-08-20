@routing @basic @testbot
Feature: Routing with weight not impacting the duration

    Background:
        Given the profile "testbot"

    Scenario: Avoid the fastest route
        Given the node map
            | a |   | b |
            |   | c |   |

        And the ways
            | nodes | weight |
            | ab    | avoid  |
            | ac    |        |
            | cb    |        |

        When I route I should get
            | from | to | route | time |
            | a    | b  | ac,cb | 28s  |

    Scenario: Avoid the fastest route
        Given the node map
            | a |   | b |
            |   | c |   |

        And the ways
            | nodes | weight |
            | ab    |        |
            | ac    | prefer |
            | cb    | prefer |

        When I route I should get
            | from | to | route | time |
            | a    | b  | ac,cb | 28s  |

    Scenario: Reference route with no distinct weight
        Given the node map
            | a |   | b |
            |   | c |   |

        And the ways
            | nodes | weight |
            | ab    |        |
            | ac    |        |
            | cb    |        |

        When I route I should get
            | from | to | route | time |
            | a    | b  | ab    | 20s  |
