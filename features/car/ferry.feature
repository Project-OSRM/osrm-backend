@routing @car @ferry
Feature: Car - Handle ferry routes

    Background:
        Given the profile "car"

    Scenario: Car - Use a ferry route
        Given the node map
            | a | b | c |   |   |
            |   |   | d |   |   |
            |   |   | e | f | g |

        And the ways
            | nodes | highway | route | bicycle |
            | abc   | primary |       |         |
            | cde   |         | ferry | yes     |
            | efg   | primary |       |         |

        When I route I should get
            | from | to | route       | modes                 |
            | a    | g  | abc,cde,efg | driving,ferry,driving |
            | b    | f  | abc,cde,efg | driving,ferry,driving |
            | e    | c  | cde         | ferry                 |
            | e    | b  | cde,abc     | ferry,driving         |
            | e    | a  | cde,abc     | ferry,driving         |
            | c    | e  | cde         | ferry                 |
            | c    | f  | cde,efg     | ferry,driving         |
            | c    | g  | cde,efg     | ferry,driving         |

    Scenario: Car - Properly handle simple durations
        Given the node map
            | a | b | c |   |   |
            |   |   | d |   |   |
            |   |   | e | f | g |

        And the ways
            | nodes | highway | route | duration |
            | abc   | primary |       |          |
            | cde   |         | ferry | 00:01:00 |
            | efg   | primary |       |          |

        When I route I should get
            | from | to | route       | modes                 | speed   |
            | a    | g  | abc,cde,efg | driving,ferry,driving | 25 km/h |
            | b    | f  | abc,cde,efg | driving,ferry,driving | 20 km/h |
            | c    | e  | cde         | ferry                 | 12 km/h |
            | e    | c  | cde         | ferry                 | 12 km/h |

    Scenario: Car - Properly handle ISO 8601 durations
        Given the node map
            | a | b | c |   |   |
            |   |   | d |   |   |
            |   |   | e | f | g |

        And the ways
            | nodes | highway | route | duration |
            | abc   | primary |       |          |
            | cde   |         | ferry | PT1M     |
            | efg   | primary |       |          |

        When I route I should get
            | from | to | route       | modes                 | speed   |
            | a    | g  | abc,cde,efg | driving,ferry,driving | 25 km/h |
            | b    | f  | abc,cde,efg | driving,ferry,driving | 20 km/h |
            | c    | e  | cde         | ferry                 | 12 km/h |
            | e    | c  | cde         | ferry                 | 12 km/h |
