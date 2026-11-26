@routing @car @weight
Feature: Car - weights

    Scenario: Only routes down service road when that's the destination
        Given the profile "car"
        And the node map
            """
            a--b--c
               |  |
               d  |
               |  |
            e--f--g
            """
        And the ways
            | nodes | highway     |
            | abc   | residential |
            | efg   | residential |
            | cg    | tertiary    |
            | bdf   | service     |
        When I route I should get
            | from | to | route          | speed   | weight |
            | a    | e  | abc,cg,efg,efg | 29 km/h | 122.4  |
            | a    | d  | abc,bdf,bdf    | 18 km/h | 71.7   |

    Scenario: Does not jump off the highway to go down service road
        Given the profile "car"
        And the node map
            """
            a
            |
            b
            |\
            | e
            |/
            c
            |
            d
            """
        And the nodes
            | node | id |
            | a    | 1  |
            | b    | 2  |
            | c    | 3  |
            | d    | 4  |
            | e    | 5  |
        And the ways
            | nodes | highway | oneway |
            | ab    | primary | yes    |
            | bc    | primary | yes    |
            | cd    | primary | yes    |
            | be    | service | yes    |
            | ec    | service | yes    |
        And the contract extra arguments "--segment-speed-file {speeds_file}"
        And the customize extra arguments "--segment-speed-file {speeds_file}"
        And the speed file
            """
            2,5,8
            """
        When I route I should get
            | from | to | route       | speed   | weight |
            | a    | d  | ab,bc,cd,cd | 65 km/h | 44.4   |
            | a    | e  | ab,be,be    | 14 km/h | 112.2  |

    Scenario: Distance weights
        Given the profile file "car" initialized with
        """
        profile.properties.weight_name = 'distance'
        """

        Given the node map
            """
            a---b---c
                |
                d
            """

        And the ways
            | nodes |
            | abc   |
            | bd    |

        When I route I should get
            | waypoints | bearings | route     | distance | weights   | times          |
            | a,b       | 90 90    | abc,abc   | 200m     | 200,0     | 11.1s,0s       |
            | b,c       | 90 90    | abc,abc   | 200m     | 200,0     | 11.1s,0s       |
            | a,d       | 90 180   | abc,bd,bd | 400m     | 200,200,0 | 13.2s,11.1s,0s |

    Scenario: CO2 weights prefer moderate speeds
        # three parallel routes from a to b: ab residential 9 km (25 km/h, fewest meters),
        # apb primary 10.5 km (64 km/h, lowest emissions), amb motorway 12.5 km (104 km/h, fastest)
        Given the profile file "car" initialized with
        """
        profile.properties.weight_name = 'co2'
        """

        Given the node locations
            | node | lat      | lon     |
            | s    | 45.00000 | 4.99746 |
            | a    | 45.00000 | 5.00000 |
            | b    | 45.00000 | 5.11434 |
            | e    | 45.00000 | 5.11688 |
            | p    | 45.02446 | 5.05717 |
            | m    | 44.96077 | 5.05717 |

        And the ways
            | nodes | highway     | maxspeed |
            | sa    | residential |          |
            | be    | residential |          |
            | ab    | residential |          |
            | apb   | primary     | 80       |
            | amb   | motorway    | 130      |

        When I route I should get
            | from | to | route        | weight |
            | s    | e  | sa,apb,be,be | 1441.6 |

    Scenario: Routability CO2 weights prefer moderate speeds
        Given the profile file "car" initialized with
        """
        profile.properties.weight_name = 'routability-co2'
        """

        Given the node locations
            | node | lat      | lon     |
            | s    | 45.00000 | 4.99746 |
            | a    | 45.00000 | 5.00000 |
            | b    | 45.00000 | 5.11434 |
            | e    | 45.00000 | 5.11688 |
            | p    | 45.02446 | 5.05717 |
            | m    | 44.96077 | 5.05717 |

        And the ways
            | nodes | highway     | maxspeed |
            | sa    | residential |          |
            | be    | residential |          |
            | ab    | residential |          |
            | apb   | primary     | 80       |
            | amb   | motorway    | 130      |

        When I route I should get
            | from | to | route        | weight |
            | s    | e  | sa,apb,be,be | 1441.6 |

    Scenario: CO2 weights ignore accessibility penalties
        # same map, the primary road is narrow (width=3): 'co2' ignores the accessibility
        # penalty, 'routability-co2' applies it and falls back to the residential road
        Given the profile file "car" initialized with
        """
        profile.properties.weight_name = 'co2'
        """

        Given the node locations
            | node | lat      | lon     |
            | s    | 45.00000 | 4.99746 |
            | a    | 45.00000 | 5.00000 |
            | b    | 45.00000 | 5.11434 |
            | e    | 45.00000 | 5.11688 |
            | p    | 45.02446 | 5.05717 |
            | m    | 44.96077 | 5.05717 |

        And the ways
            | nodes | highway     | maxspeed | width |
            | sa    | residential |          |       |
            | be    | residential |          |       |
            | ab    | residential |          |       |
            | apb   | primary     | 80       | 3     |
            | amb   | motorway    | 130      |       |

        When I route I should get
            | from | to | route        | weight |
            | s    | e  | sa,apb,be,be | 1441.6 |

    Scenario: Routability CO2 weights apply accessibility penalties
        Given the profile file "car" initialized with
        """
        profile.properties.weight_name = 'routability-co2'
        """

        Given the node locations
            | node | lat      | lon     |
            | s    | 45.00000 | 4.99746 |
            | a    | 45.00000 | 5.00000 |
            | b    | 45.00000 | 5.11434 |
            | e    | 45.00000 | 5.11688 |
            | p    | 45.02446 | 5.05717 |
            | m    | 44.96077 | 5.05717 |

        And the ways
            | nodes | highway     | maxspeed | width |
            | sa    | residential |          |       |
            | be    | residential |          |       |
            | ab    | residential |          |       |
            | apb   | primary     | 80       | 3     |
            | amb   | motorway    | 130      |       |

        When I route I should get
            | from | to | route       | weight |
            | s    | e  | sa,ab,be,be | 1711.1 |

    Scenario: CO2 weights on a ferry follow the duration on board
        Given the profile file "car" initialized with
        """
        profile.properties.weight_name = 'co2'
        """

        Given the node locations
            | node | lat      | lon     |
            | s    | 45.00000 | 4.99746 |
            | a    | 45.00000 | 5.00000 |
            | b    | 45.00000 | 5.11434 |
            | e    | 45.00000 | 5.11688 |

        And the ways
            | nodes | highway     | route | duration |
            | sa    | residential |       |          |
            | be    | residential |       |          |
            | ab    | (nil)       | ferry | 00:30    |

        When I route I should get
            | from | to | route       | weight |
            | s    | e  | sa,ab,be,be | 972.8  |
