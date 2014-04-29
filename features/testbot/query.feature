@routing @query
Feature: Query message parsing

    Background:
        Given the profile "testbot"

    Scenario: Malformed requests
        Given the node locations
            | node | lat  | lon  |
            | a    | 1.00 | 1.00 |
            | b    | 1.01 | 1.00 |

        And the ways
            | nodes |
            | ab    |

        When I route I should get
            | request                     | status | message                                    |
            | viaroute?loc=1,1&loc=1.01,1 | 0      | Found route between points                 |
            | nonsense                    | 400    | Bad Request                                |
            | nonsense?loc=1,1&loc=1.01,1 | 400    | Bad Request                                |
            |                             | 400    | Query string malformed close to position 0 |
            | /                           | 400    | Query string malformed close to position 0 |
            | ?                           | 400    | Query string malformed close to position 0 |
            | viaroute/loc=               | 400    | Query string malformed close to position 9 |
            | viaroute/loc=1              | 400    | Query string malformed close to position 9 |
            | viaroute/loc=1,1            | 400    | Query string malformed close to position 9 |
            | viaroute/loc=1,1,1          | 400    | Query string malformed close to position 9 |
            | viaroute/loc=x              | 400    | Query string malformed close to position 9 |
            | viaroute/loc=x,y            | 400    | Query string malformed close to position 9 |
            | viaroute/loc=1,1&loc=       | 400    | Query string malformed close to position 9 |
            | viaroute/loc=1,1&loc=1      | 400    | Query string malformed close to position 9 |
            | viaroute/loc=1,1&loc=1,1    | 400    | Query string malformed close to position 9 |
            | viaroute/loc=1,1&loc=1,1,1  | 400    | Query string malformed close to position 9 |
            | viaroute/loc=1,1&loc=x      | 400    | Query string malformed close to position 9 |
            | viaroute/loc=1,1&loc=x,y    | 400    | Query string malformed close to position 9 |