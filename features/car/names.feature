@routing @car @names
Feature: Car - Street names in instructions

    Background:
        Given the profile "car"

    Scenario: Car - A named street
        Given the node map
            | a | b |
            |   | c |

        And the ways
            | nodes | name     | ref |
            | ab    | My Way   |     |
            | bc    | Your Way | A1  |

        When I route I should get
            | from | to | route                              |
            | a    | c  | My Way,Your Way (A1),Your Way (A1) |

    Scenario: Car - A named street with pronunciation
        Given the node map
            | a | b | d |
            |   | 1 |   |
            |   | c |   |

        And the ways
            | nodes | name     |name:pronunciation | ref |
            | ab    | My Way   |                    |     |
            | bd    | My Way   | meyeway            | A1  |
            | cd    | Your Way | yourewaye          |     |

        When I route I should get
            | from | to | route              | pronunciations      |
            | a    | d  | My Way,My Way (A1) | ,meyeway             |
            | 1    | c  | Your Way,Your Way  | yourewaye,yourewaye  |

    @todo
    Scenario: Car - Use way type to describe unnamed ways
        Given the node map
            | a | b | c | d |

        And the ways
            | nodes | highway     | name |
            | ab    | tertiary    |      |
            | bcd   | residential |      |

        When I route I should get
            | from | to | route                            |
            | a    | c  | tertiary,residential,residential |
