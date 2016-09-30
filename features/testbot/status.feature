@routing @status @testbot
Feature: Status messages

    Background:
        Given the profile "testbot"

    Scenario: Route found
        Given the node map
            """
            a b
            """

        Given the ways
            | nodes |
            | ab    |

        When I route I should get
            | from | to | route | status | message                    |
            | a    | b  | ab,ab | 200    |                            |
            | b    | a  | ab,ab | 200    |                            |

    Scenario: No route found
        Given the node map
            """
            a b

            c d
            """

        Given the ways
            | nodes |
            | ab    |
            | cd    |

        When I route I should get
            | from | to | route | status | message                          |
            | a    | b  | ab,ab | 200    |                                  |
            | c    | d  | cd,cd | 200    |                                  |
            | a    | c  |       | 400    | Impossible route between points  |
            | b    | d  |       | 400    | Impossible route between points  |

    Scenario: Malformed requests
        Given the node locations
            | node | lat  | lon  |
            | a    | 1.00 | 1.00 |
            | b    | 2.00 | 1.00 |

        And the ways
            | nodes |
            | ab    |

        When I route I should get
            | request                             | status | message                                           |
            | route/v1/driving/1,1;1,2            | 200    |                                                   |
            | route/v1/driving/-74697224,5.191564 | 400    | Query string malformed close to position 18       |
            | route/v1/driving/200,5.191564;44,5  | 400    | Invalid coordinate value.                         |
            | nonsense                            | 400    | URL string malformed close to position 9: "nse"   |
            | nonsense/v1/driving/1,1;1,2         | 400    | Service nonsense not found!                       |
            |                                     | 400    | URL string malformed close to position 1: "/"     |
            | /                                   | 400    | URL string malformed close to position 1: "//"    |
            | ?                                   | 400    | URL string malformed close to position 1: "/?"    |
            | route/v1/driving                    | 400    | URL string malformed close to position 17: "ing"  |
            | route/v1/driving/                   | 400    | URL string malformed close to position 18: "ng/"  |
            | route/v1/driving/1                  | 400    | Query string malformed close to position 19       |
            | route/v1/driving/1,1                | 400    | Number of coordinates needs to be at least two.   |
            | route/v1/driving/1,1,1              | 400    | Query string malformed close to position 21       |
            | route/v1/driving/x                  | 400    | Query string malformed close to position 18       |
            | route/v1/driving/x,y                | 400    | Query string malformed close to position 18       |
            | route/v1/driving/1,1;               | 400    | Query string malformed close to position 21       |
            | route/v1/driving/1,1;1              | 400    | Query string malformed close to position 23       |
            | route/v1/driving/1,1;1,1,1          | 400    | Query string malformed close to position 25       |
            | route/v1/driving/1,1;x              | 400    | Query string malformed close to position 21       |
            | route/v1/driving/1,1;x,y            | 400    | Query string malformed close to position 21       |
