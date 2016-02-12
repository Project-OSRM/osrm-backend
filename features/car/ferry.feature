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
            | from | to | route       | modes |
            | a    | g  | abc,cde,efg | 2,5,2 |
            | b    | f  | abc,cde,efg | 2,5,2 |
            | e    | c  | cde         | 5     |
            | e    | b  | cde,abc     | 5,2   |
            | e    | a  | cde,abc     | 5,2   |
            | c    | e  | cde         | 5     |
            | c    | f  | cde,efg     | 5,2   |
            | c    | g  | cde,efg     | 5,2   |

    Scenario: Car - Handle different durations formats
        Given the node map
            | a | b |
            | c | d |
            | e | f |

        And the ways
            | nodes | highway | route | duration   |
            | ab    |         | ferry | PT1H01M1S  |
            | cd    |         | ferry | 01:01:01   | 
            | ef    |         | ferry | PT01:01:01 |

        When I route I should get
            | from | to | route | time  |
            | a    | b  | ab    | 3661s |
            | c    | d  | cd    | 3661s |
            | e    | f  | ef    | 3661s |
