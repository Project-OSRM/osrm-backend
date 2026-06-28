@routing @foot @conveying
Feature: Foot - Conveying tag on escalators and moving walkways
# Reference: https://wiki.openstreetmap.org/wiki/Key:conveying

    Background:
        Given the profile "foot"
        Given a grid size of 10 meters

    Scenario: Foot - Escalator with conveying=forward: only traversable in forward direction
        Given the node map
            """
            a b c
            """

        And the ways
            | nodes | highway | conveying |
            | abc   | steps   | forward   |

        When I route I should get
            | from | to | route   | time  |
            | a    | c  | abc,abc | 14.4s |

        When I route I should get
            | from | to | status |
            | c    | a  | 400    |

    Scenario: Foot - Escalator with conveying=backward: only traversable in backward direction
        Given the node map
            """
            a b c
            """

        And the ways
            | nodes | highway | conveying |
            | abc   | steps   | backward  |

        When I route I should get
            | from | to | route   | time  |
            | c    | a  | abc,abc | 14.4s |

        When I route I should get
            | from | to | status |
            | a    | c  | 400    |

    Scenario: Foot - Escalator with conveying=reversible: equal time both ways
        Given the node map
            """
            a b c
            """

        And the ways
            | nodes | highway | conveying  |
            | abc   | steps   | reversible |

        When I route I should get
            | from | to | route   | time  |
            | a    | c  | abc,abc | 14.4s |
            | c    | a  | abc,abc | 14.4s |

    Scenario: Foot - Moving walkway with conveying=forward: only traversable in forward direction
        Given the node map
            """
            a b c
            """

        And the ways
            | nodes | highway  | conveying |
            | abc   | footway  | forward   |

        When I route I should get
            | from | to | route   | time  |
            | a    | c  | abc,abc | 14.4s |

        When I route I should get
            | from | to | status |
            | c    | a  | 400    |

    Scenario: Foot - No conveying tag: equal time both directions
        Given the node map
            """
            a b c
            """

        And the ways
            | nodes | highway |
            | abc   | steps   |

        When I route I should get
            | from | to | route   | time  |
            | a    | c  | abc,abc | 14.4s |
            | c    | a  | abc,abc | 14.4s |

    Scenario: Foot - Conveying=yes without direction: equal time both directions
        Given the node map
            """
            a b c
            """

        And the ways
            | nodes | highway | conveying |
            | abc   | steps   | yes       |

        When I route I should get
            | from | to | route   | time  |
            | a    | c  | abc,abc | 14.4s |
            | c    | a  | abc,abc | 14.4s |

    Scenario: Foot - Conveying tag ignored on non-escalator highways: equal time both directions
        Given the node map
            """
            a b c
            """

        And the ways
            | nodes | highway   | conveying |
            | abc   | residential | forward   |

        When I route I should get
            | from | to | route   | time  |
            | a    | c  | abc,abc | 14.4s |
            | c    | a  | abc,abc | 14.4s |

    Scenario: Foot - Escalator with oneway=no allows bidirectional travel
        Given the node map
            """
            a b c
            """

        And the ways
            | nodes | highway | conveying | oneway |
            | abc   | steps   | forward   | no     |

        When I route I should get
            | from | to | route   | time  |
            | a    | c  | abc,abc | 14.4s |
            | c    | a  | abc,abc | 14.4s |

    Scenario: Foot - Prefer escalator with favorable conveying direction
        Given the node map
            """
            a b c
            d e f
            """

        And the ways
            | nodes | highway  | conveying |
            | ad    | footway  |           |
            | de    | footway  |           |
            | ef    | footway  |           |
            | abc   | steps    | forward   |

        When I route I should get
            | from | to | route       |
            | a    | c  | abc,abc     |

        When I route I should get
            | from | to | status |
            | c    | a  | 400    |
