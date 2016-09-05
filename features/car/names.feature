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
            | from | to | route                              | ref  |
            | a    | c  | My Way,Your Way,Your Way           | ,A1,A1|

    Scenario: Car - A named street with pronunciation
        Given the node map
            | a | b | d |
            |   | 1 |   |
            |   | c |   |

        And the ways
            | nodes | name     |name:pronunciation  | ref |
            | ab    | My Way   |                    |     |
            | bd    | My Way   | meyeway            | A1  |
            | cd    | Your Way | yourewaye          |     |

        When I route I should get
            | from | to | route              | pronunciations       | ref   |
            | a    | d  | My Way,My Way      | ,meyeway             | ,A1   |
            | 1    | c  | Your Way,Your Way  | yourewaye,yourewaye  | ,     |

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

    Scenario: Inner city expressway with on road
        Given the node map
            | a | b |   |   |   | c | g |
            |   |   |   |   | f |   |   |
            |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |
            |   |   |   |   |   | d |   |
            |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |
            |   |   |   |   |   | e |   |

        And the ways
            | nodes | highway      | name  | name:pronunciation |
            | abc   | primary      | road  | roooaad            |
            | cg    | primary      | road  | roooaad            |
            | bfd   | trunk_link   |       |                    |
            | cde   | trunk        | trunk | truank             |

        And the relations
            | type        | way:from | way:to | node:via | restriction   |
            | restriction | abc      | cde    | c        | no_right_turn |

       When I route I should get
            | waypoints | route                | turns                    | pronunciations        |
            | a,e       | road,trunk,trunk     | depart,turn right,arrive | roooaad,truank,truank |
