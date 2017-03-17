@routing @car @weight
Feature: Car - weights

    Background: Use specific speeds
        Given the profile "car"

    Scenario: Only routes down service road when that's the destination
        Given the node map
            """
            a--b--c
               |
               d
               |
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
            | a    | e  | abc,cg,efg,efg | 28 km/h | 126.6  |
            | a    | d  | abc,bdf,bdf    | 18 km/h | 71.7   |

    Scenario: Does not jump off the highway to go down service road
        Given the node map
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
        And the extract extra arguments "--generate-edge-lookup"
        And the contract extra arguments "--segment-speed-file {speeds_file}"
        And the customize extra arguments "--segment-speed-file {speeds_file}"
        And the speed file
            """
            2,5,8
            """
        When I route I should get
            | from | to | route       | speed   | weight |
            | a    | d  | ab,bc,cd,cd | 65 km/h | 44.4   |
            | a    | e  | ab,be,be    | 14 km/h | 112    |
