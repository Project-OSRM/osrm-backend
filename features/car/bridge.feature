@routing @car @bridge
Feature: Car - Handle driving

    Background:
        Given the profile "car"
        Given a grid size of 200 meters

    Scenario: Car - Use a ferry route
        Given the node map
            """
            a b c
                d
                e f g
            """

        And the ways
            | nodes | highway | bridge  | bicycle |
            | abc   | primary |         |         |
            | cde   | primary | movable | yes     |
            | efg   | primary |         |         |

        When I route I should get
            | from | to | route           | modes                           | turns                                      |
            | a    | g  | abc,cde,efg,efg | driving,driving,driving,driving | depart,new name right,new name left,arrive |
            | e    | a  | cde,abc,abc     | driving,driving,driving         | depart,new name left,arrive                |

    Scenario: Car - Control test without durations, osrm uses movable bridge speed to calculate duration
        Given the node map
            """
            a b c
                d
                e f g
            """

        And the ways
            | nodes | highway | bridge  |
            | abc   | primary |         |
            | cde   | primary | movable |
            | efg   | primary |         |

        When I route I should get
            | from | to | route           | modes                           | speed   | time     | turns                                      |
            | a    | g  | abc,cde,efg,efg | driving,driving,driving,driving | 13 km/h | 332s +-1 | depart,new name right,new name left,arrive |
            | e    | c  | cde,cde         | driving,driving                 | 5 km/h  | 288s +-1 | depart,arrive                              |

    Scenario: Car - Properly handle durations
        Given the node map
            """
            a b c
                d
                e f g
            """

        And the ways
            | nodes | highway | bridge  | duration |
            | abc   | primary |         |          |
            | cde   | primary | movable | 00:10:00 |
            | efg   | primary |         |          |

        When I route I should get
            | from | to | route           | modes                           | speed  | turns                                      |
            | a    | g  | abc,cde,efg,efg | driving,driving,driving,driving | 7 km/h | depart,new name right,new name left,arrive |
            | c    | e  | cde,cde         | driving,driving                 | 2 km/h | depart,arrive                              |
            | e    | c  | cde,cde         | driving,driving                 | 2 km/h | depart,arrive                              |
