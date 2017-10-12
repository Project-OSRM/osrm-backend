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
            | route/v1/driving/-74697224,5.191564 | 400    | At least two coordinates must be provided |
            | route/v1/driving/200,5.191564;44,5  | 400    | Lng/Lat coordinates must be within world bounds (-180 < lng < 180, -90 < lat < 90) |
            | nonsense                            | 404    | Path not found   |
            | nonsense/v1/driving/1,1;1,2         | 404    | Path not found |
            |                                     | 404    | Path not found |
            | /                                   | 404    | Path not found |
            | ?                                   | 404    | Path not found |
            | route/v1/driving                    | 404    | Path not found |
            | route/v1/driving/                   | 404    | Path not found |
            | route/v1/driving/1                  | 400    | At least two coordinates must be provided |
            | route/v1/driving/1,1                | 400    | At least two coordinates must be provided |
            | route/v1/driving/1,1,1              | 400    | At least two coordinates must be provided |
            | route/v1/driving/x                  | 400    | At least two coordinates must be provided |
            | route/v1/driving/x,y                | 400    | At least two coordinates must be provided |
            | route/v1/driving/1,1;               | 400    | Coordinates must be an array of (lon/lat) pairs |
            | route/v1/driving/1,1;1              | 400    | Coordinates must be an array of (lon/lat) pairs |
            | route/v1/driving/1,1;1,1,1          | 400    | Coordinates must be an array of (lon/lat) pairs |
            | route/v1/driving/1,1;x              | 400    | Coordinates must be an array of (lon/lat) pairs |
            | route/v1/driving/1,1;x,y            | 400    | Lng/Lat coordinates must be valid numbers |
