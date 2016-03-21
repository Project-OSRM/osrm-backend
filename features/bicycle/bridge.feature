@routing @bicycle @bridge
Feature: Bicycle - Handle movable bridge

    Background:
        Given the profile "bicycle"

    Scenario: Car - Use a movable bridge route
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
            | a    | g  | abc,cde,efg | 3,7,3 |
            | b    | f  | abc,cde,efg | 3,7,3 |
            | e    | c  | cde         | 7     |
            | e    | b  | cde,abc     | 7,3   |
            | e    | a  | cde,abc     | 7,3   |
            | c    | e  | cde         | 7     |
            | c    | f  | cde,efg     | 7,3   |
            | c    | g  | cde,efg     | 7,3   |

    Scenario: Car - Properly handle durations
        Given the node map
            | a | b | c |   |   |
            |   |   | d |   |   |
            |   |   | e | f | g |

        And the ways
            | nodes | highway | bridge  | duration |
            | abc   | primary |         |          |
            | cde   |         | movable | 00:05:00 |
            | efg   | primary |         |          |

        When I route I should get
            | from | to | route       | modes | speed   |
            | a    | g  | abc,cde,efg | 3,7,3 | 5 km/h |
            | b    | f  | abc,cde,efg | 3,7,3 | 4 km/h |
            | c    | e  | cde         | 7     | 2 km/h |
            | e    | c  | cde         | 7     | 2 km/h |
