@routing @car @bridge
Feature: Car - Handle movable bridge

    Background:
        Given the profile "car"

    Scenario: Car - Use a movable bridge
        Given the node map
            | a | b | c |   |   |
            |   |   | d |   |   |
            |   |   | e | f | g |

        And the ways
            | nodes | highway | bridge  | bicycle |
            | abc   | primary |         |         |
            | cde   |         | movable | yes     |
            | efg   | primary |         |         |

        When I route I should get
            | from | to | route       | modes |
            | a    | g  | abc,cde,efg | 2,7,2 |
            | b    | f  | abc,cde,efg | 2,7,2 |
            | e    | c  | cde         | 7     |
            | e    | b  | cde,abc     | 7,2   |
            | e    | a  | cde,abc     | 7,2   |
            | c    | e  | cde         | 7     |
            | c    | f  | cde,efg     | 7,2   |
            | c    | g  | cde,efg     | 7,2   |

    Scenario: Car - Duration on movable bridges
        Given the node map
            | a | b |
            | c | d |
            | e | f |

        And the ways
            | nodes | bridge  | duration   |
            | ab    | movable | PT1H01M1S  |
            | cd    | movable | 01:01:01   | 
            | ef    | movable | PT01:01:01 |

        When I route I should get
            | from | to | route | time  |
            | a    | b  | ab    | 3661s |
            | c    | d  | cd    | 3661s |
            | e    | f  | ef    | 3661s |
