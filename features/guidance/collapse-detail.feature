@routing  @guidance @collapsing
Feature: Collapse

    Background:
        Given the profile "car"
        Given a grid size of 5 meters

    @reverse
    Scenario: Collapse U-Turn Triangle Intersection
        Given the node map
            """
            g   f   e   d


            a     b     c
            """

        And the ways
            | nodes | highway      | name | oneway |
            | abc   | primary      | road | yes    |
            | defg  | primary      | road | yes    |
            | fb    | primary_link |      | yes    |
            | be    | primary_link |      | yes    |

       When I route I should get
            | waypoints | route          | turns                        |
            | a,g       | road,road,road | depart,continue uturn,arrive |
            | d,c       | road,road,road | depart,continue uturn,arrive |

    @reverse @traffic-signals
    Scenario: Collapse U-Turn Triangle Intersection
        Given the node map
            """
            g   f   j   e   d

                  h   i

            a       b       c
            """

        And the ways
            | nodes | highway      | name | oneway |
            | abc   | primary      | road | yes    |
            | dejfg | primary      | road | yes    |
            | fhb   | primary_link |      |        |
            | bie   | primary_link |      |        |

       And the nodes
            | node | highway         |
            | j    | traffic_signals |
            | h    | traffic_signals |
            | i    | traffic_signals |

       When I route I should get
            | waypoints | route          | turns                        |
            | a,g       | road,road,road | depart,continue uturn,arrive |
            | d,c       | road,road,road | depart,continue uturn,arrive |
