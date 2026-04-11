@routing @foot @conveying
Feature: Foot - Conveying tag on escalators and moving walkways
# Reference: https://wiki.openstreetmap.org/wiki/Key:conveying

    Background:
        Given the profile "foot"
        Given a grid size of 10 meters

    Scenario: Foot - Escalator with conveying=forward is routable both directions
        Given the node map
            """
            a b c
            """

        And the ways
            | nodes | highway | conveying |
            | abc   | steps   | forward   |

        When I route I should get
            | from | to | route   |
            | a    | c  | abc,abc |
            | c    | a  | abc,abc |

    Scenario: Foot - Escalator with conveying=backward is routable both directions
        Given the node map
            """
            a b c
            """

        And the ways
            | nodes | highway | conveying |
            | abc   | steps   | backward  |

        When I route I should get
            | from | to | route   |
            | a    | c  | abc,abc |
            | c    | a  | abc,abc |

    Scenario: Foot - Escalator with conveying=reversible works both ways
        Given the node map
            """
            a b c
            """

        And the ways
            | nodes | highway | conveying  |
            | abc   | steps   | reversible |

        When I route I should get
            | from | to | route   |
            | a    | c  | abc,abc |
            | c    | a  | abc,abc |

    Scenario: Foot - Moving walkway with conveying=forward
        Given the node map
            """
            a b c
            """

        And the ways
            | nodes | highway  | conveying |
            | abc   | footway  | forward   |

        When I route I should get
            | from | to | route   |
            | a    | c  | abc,abc |
            | c    | a  | abc,abc |

    Scenario: Foot - No conveying tag remains bidirectional
        Given the node map
            """
            a b c
            """

        And the ways
            | nodes | highway |
            | abc   | steps   |

        When I route I should get
            | from | to | route   |
            | a    | c  | abc,abc |
            | c    | a  | abc,abc |

    Scenario: Foot - Conveying=yes without direction remains bidirectional
        Given the node map
            """
            a b c
            """

        And the ways
            | nodes | highway | conveying |
            | abc   | steps   | yes       |

        When I route I should get
            | from | to | route   |
            | a    | c  | abc,abc |
            | c    | a  | abc,abc |

    Scenario: Foot - Conveying tag is ignored on non-step, non-footway highways
        Given the node map
            """
            a b c
            """

        And the ways
            | nodes | highway   | conveying |
            | abc   | residential | forward   |

        When I route I should get
            | from | to | route   |
            | a    | c  | abc,abc |
            | c    | a  | abc,abc |

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
            | c    | a  | abc,abc     |
