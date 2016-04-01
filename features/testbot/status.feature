@routing @status @testbot
Feature: Status messages

    Background:
        Given the profile "testbot"

    Scenario: Route found
        Given the node map
            | a | b |

        Given the ways
            | nodes |
            | ab    |

        When I route I should get
            | from | to | route | status | message                    |
            | a    | b  | ab,ab | 200    |                            |
            | b    | a  | ab,ab | 200    |                            |

    Scenario: No route found
        Given the node map
            | a | b |
            |   |   |
            | c | d |

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
            | request                     | status | message                                         |
            | route/v1/driving/1,1;1,2    | 200    |                                                 |
            | nonsense                    | 400    | URL string malformed close to position 0: "/no" |
            | nonsense/v1/driving/1,1;1,2 | 400    | Service nonsense not found!                     |
            |                             | 400    | URL string malformed close to position 0: "/"   |
            | /                           | 400    | URL string malformed close to position 0: "//"  |
            | ?                           | 400    | URL string malformed close to position 0: "/?"  |
            | route/v1/driving            | 400    | URL string malformed close to position 0: "/ro" |
            | route/v1/driving/           | 400    | URL string malformed close to position 0: "/ro" |
            | route/v1/driving/1          | 400    | Query string malformed close to position 0      |
            | route/v1/driving/1,1        | 400    | Number of coordinates needs to be at least two. |
            | route/v1/driving/1,1,1      | 400    | Query string malformed close to position 3      |
            | route/v1/driving/x          | 400    | Query string malformed close to position 0      |
            | route/v1/driving/x,y        | 400    | Query string malformed close to position 0      |
            | route/v1/driving/1,1;       | 400    | Query string malformed close to position 3      |
            | route/v1/driving/1,1;1      | 400    | Query string malformed close to position 3      |
            | route/v1/driving/1,1;1,1,1  | 400    | Query string malformed close to position 7      |
            | route/v1/driving/1,1;x      | 400    | Query string malformed close to position 3      |
            | route/v1/driving/1,1;x,y    | 400    | Query string malformed close to position 3      |
