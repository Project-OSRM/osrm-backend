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
            | b    | 1.01 | 1.00 |

        And the ways
            | nodes |
            | ab    |

        When I route I should get
            | request                     | status | message                                     |
            | viaroute?loc=1,1&loc=1.01,1 | 200    |                                             |
            | nonsense                    | 400    | Service not found                           |
            | nonsense?loc=1,1&loc=1.01,1 | 400    | Service not found                           |
            |                             | 400    | Query string malformed close to position 0  |
            | /                           | 400    | Query string malformed close to position 0  |
            | ?                           | 400    | Query string malformed close to position 0  |
            | viaroute?loc=               | 400    | Query string malformed close to position 9  |
            | viaroute?loc=1              | 400    | Query string malformed close to position 9  |
            | viaroute?loc=1,1            | 400    | Invalid coordinates                         |
            | viaroute?loc=1,1,1          | 400    | Query string malformed close to position 17 |
            | viaroute?loc=x              | 400    | Query string malformed close to position 9  |
            | viaroute?loc=x,y            | 400    | Query string malformed close to position 9  |
            | viaroute?loc=1,1&loc=       | 400    | Query string malformed close to position 17 |
            | viaroute?loc=1,1&loc=1      | 400    | Query string malformed close to position 17 |
            | viaroute?loc=1,1&loc=1,1    | 200    |                                             |
            | viaroute?loc=1,1&loc=1,1,1  | 400    | Query string malformed close to position 25 |
            | viaroute?loc=1,1&loc=x      | 400    | Query string malformed close to position 17 |
            | viaroute?loc=1,1&loc=x,y    | 400    | Query string malformed close to position 17 |
