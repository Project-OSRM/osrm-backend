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
            | from | to | route          | speed   |
            | a    | e  | abc,cg,efg,efg | 23 km/h |
            | a    | d  | abc,bdf,bdf    | 14 km/h |

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
        And the ways
            | nodes | highway |
            | ab    | primary |
            | bc    | primary |
            | cd    | primary |
            | be    | service |
            | ec    | service |
        And the extract extra arguments "--generate-edge-lookup"
        And the contract extra arguments "--segment-speed-file {speeds_file}"
        And the speed file
            """
            2,4,8
            """
        When I route I should get
            | from | to | route       | speed   |
            | a    | d  | ab,bc,cd,cd | 14 km/h |
            | a    | e  | ab,be,be    | 19 km/h |
