@routing @basic @car
Feature: Basic Routing

    Background:
        Given the profile "car"
        Given a grid size of 500 meters

    @smallest @via
    Scenario: Summaries when routing on a simple network
        Given the node map
            """
            b     f

            c d   g

            a   e
            """

        And the ways
            | nodes | name   |
            | acb   | road   |
            | de    | 1 st   |
            | cd    |        |
            | dg    | blvd   |
            | df    | street |

        When I route I should get
            | waypoints | route                               | summary                 |
            | a,e       | road,,1 st,1 st                     | road, 1 st              |
            # The via node `d` belongs to `cd`, `de`, `df`, `dg` and depending on the edge
            # summary can be "road;street", "road, 1 st;1 st, street", "road, blvd;blvd, street"
            # The test must be fixed by #2287
            #| a,d,f     | road,,,street,street                | road;street             |
            | a,e,f     | road,,1 st,1 st,1 st,street,street  | road, 1 st;1 st, street |

     Scenario: Name Empty
        Given the node map
            """
            a   b     c
            """

        And the ways
            | nodes | name |
            | ab    | road |
            | bc    |      |

        When I route I should get
            | waypoints | route  | summary |
            | a,c       | road,  | road    |

     Scenario: Name Empty But Ref
        Given the node map
            """
            a   b     c
            """

        And the ways
            | nodes | name | ref |
            | ab    | road |     |
            | bc    |      | 101 |

        When I route I should get
            | waypoints | route  | summary   |
            | a,c       | road,, | road, 101 |

     Scenario: Only Refs
        Given the node map
            """
            a   b     c
            """

        And the ways
            | nodes | name | ref |
            | ab    |      | 100 |
            | bc    |      | 101 |

        When I route I should get
            | waypoints | route  | summary  |
            | a,c       | ,,     | 100, 101 |

     Scenario: Single Ref
        Given the node map
            """
            a   b     c
            """

        And the ways
            | nodes | name | ref |
            | ab    |      |     |
            | bc    |      | 101 |

        When I route I should get
            | waypoints | route | summary |
            | a,c       | ,,    | 101     |

     Scenario: Nothing
        Given the node map
            """
            a   b     c
            """

        And the ways
            | nodes | name |
            | ab    |      |
            | bc    |      |

        When I route I should get
            | waypoints | route | summary |
            | a,c       | ,     |         |
