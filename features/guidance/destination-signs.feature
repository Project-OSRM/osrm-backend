@routing @guidance
Feature: Destination Signs

    Background:
        Given the profile "car"

    Scenario: Car - route name assembly with destination signs
        Given the node map
          | a | b |
          | c | d |
          | e | f |
          | g | h |
          | i | j |
          | k | l |
          | m | n |

        And the ways
          | nodes | name | ref | destination    | destination:ref |
          | ab    | AB   | E1  |                |                 |
          | cd    | CD   |     | Berlin         |                 |
          | ef    | EF   |     | Berlin         | A1              |
          | gh    |      |     | Berlin         | A1              |
          | ij    |      |     | Berlin         |                 |
          | kl    | KL   | E1  | Berlin         | A1              |
          | mn    | MN   |     | Berlin;Hamburg | A1;A2           |

        When I route I should get
          | from | to | route                                                     |
          | a    | b  | AB (E1),AB (E1)                                           |
          | c    | d  | CD (Berlin),CD (Berlin)                                   |
          | e    | f  | EF (A1: Berlin),EF (A1: Berlin)                           |
          | g    | h  | ,                                                         |
          | i    | j  | ,                                                         |
          | k    | l  | KL (E1),KL (E1)                                           |
          | m    | n  | MN (A1, A2: Berlin, Hamburg),MN (A1, A2: Berlin, Hamburg) |
