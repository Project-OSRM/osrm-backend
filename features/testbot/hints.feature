@routing @testbot
Feature: Hints handling - incoming hints are ignored but still generated

    Background:
        Given the profile "testbot"
        Given a grid size of 100 meters

    Scenario: Normal routing works without hints
        Given the node map
            """
            a b
            """

        And the ways
            | nodes |
            | ab    |

        When I route I should get
            | from | to | route    |
            | a    | b  | ab,ab    |
            | b    | a  | ab,ab    |

    Scenario: Route with bogus hints is accepted and produces correct result
        Given the node map
            """
            a b
            """

        And the ways
            | nodes |
            | ab    |

        Given the query options
            | hints | AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA;AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA |

        When I route I should get
            | from | to | route    | status |
            | a    | b  | ab,ab    | 200    |
            | b    | a  | ab,ab    | 200    |

    Scenario: Route with empty hints parameter is accepted
        Given the node map
            """
            a b
            """

        And the ways
            | nodes |
            | ab    |

        Given the query options
            | hints | ; |

        When I route I should get
            | from | to | route    | status |
            | a    | b  | ab,ab    | 200    |
            | b    | a  | ab,ab    | 200    |
