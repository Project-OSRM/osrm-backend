@routing @guidance
Feature: Features related to bugs

    Background:
        Given the profile "car"
        Given a grid size of 5 meters

    @2852
    Scenario: Loop
        Given the node map
            """
            a 1   g     b


            e           f

                        2
            d     h     c
            """

        And the ways
            | nodes | name   | oneway |
            | agb   | top    | yes    |
            | bfc   | right  | yes    |
            | chd   | bottom | yes    |
            | dea   | left   | yes    |

        And the nodes
            | node | highway         |
            | g    | traffic_signals |
            | f    | traffic_signals |
            | h    | traffic_signals |
            | e    | traffic_signals |

        When I route I should get
            | waypoints | route     | turns         |
            | 1,2       | top,right | depart,arrive |

    @3156
    Scenario: Incorrect lanes tag
        Given the node map
            """
            a b
            """

        And the ways
            | nodes | lanes |
            | ab    | 1; 2  |

        And the data has been saved to disk
        When I try to run "osrm-extract {osm_file} --profile {profile_file}"
        Then it should exit successfully

    @3418
    Scenario: Bearings should be between 0-359
        Given the node locations
            | node | lon          | lat        |
            | a    | -122.0232176 | 37.3282203 |
            | b    | -122.0232199 | 37.3302422 |
            | c    | -122.0232252 | 37.3312787 |

        And the ways
            | nodes | name               | highway     |
            | ab    | Pear to Merrit     | residential |
            | bc    | Merritt to Apricot | residential |

        When I route I should get
            | waypoints | route | intersections  |
            | a,c       | Pear to Merrit,Merritt to Apricot,Merritt to Apricot | true:0;true:0 false:180;true:180  |
