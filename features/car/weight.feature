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
            | a    | e  | ab,be,be    | 14 km/h | 111.8  |

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
            | a,d       | 90 180   | abc,bd,bd | 399.9m   | 200,200,0 | 13.2s,11.1s,0s |
