@routing @load @testbot
Feature: Ways of loading data
# Several scenarios that change between direct/datastore makes
# it easier to check that the test framework behaves as expected.

    Background:
        Given the profile "testbot.lua"

    Scenario: Load data with datastore - ab
        Given data is loaded with datastore
        Given the node map
            """
            a b
            """

        And the ways
            | nodes |
            | ab    |

        When I route I should get
            | from | to | route |
            | a    | b  | ab,ab |
            | b    | a  | ab,ab |

    Scenario: Load data directly - st
        Given data is loaded directly
        Given the node map
            """
            s t
            """

        And the ways
            | nodes |
            | st    |

        When I route I should get
            | from | to | route |
            | s    | t  | st,st |
            | t    | s  | st,st |

    Scenario: Load data datastore - xy
        Given data is loaded with datastore
        Given the node map
            """
            x y
            """

        And the ways
            | nodes |
            | xy    |

        When I route I should get
            | from | to | route |
            | x    | y  | xy,xy |
            | y    | x  | xy,xy |

    Scenario: Load data directly - cd
        Given data is loaded directly
        Given the node map
            """
            c d
            """

        And the ways
            | nodes |
            | cd    |

        When I route I should get
            | from | to | route |
            | c    | d  | cd,cd |
            | d    | c  | cd,cd |
