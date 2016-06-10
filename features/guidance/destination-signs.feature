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
          | o | p |
          | q | r |

        And the ways
          | nodes | name | ref | destination    | destination:ref | oneway | #                                    |
          | ab    | AB   | E1  |                |                 | yes    |                                      |
          | cd    | CD   |     | Berlin         |                 | yes    |                                      |
          | ef    | EF   |     | Berlin         | A1              | yes    |                                      |
          | gh    |      |     | Berlin         | A1              | yes    |                                      |
          | ij    |      |     | Berlin         |                 | yes    |                                      |
          | kl    | KL   | E1  | Berlin         | A1              | yes    |                                      |
          | mn    | MN   |     | Berlin;Hamburg | A1;A2           | yes    |                                      |
          | op    | OP   |     | Berlin;Hamburg | A1;A2           | no     | mis-tagged destination: not a oneway |
          | qr    | QR   |     |                | A1;A2           | yes    |                                      |

        When I route I should get
          | from | to | route                                                     | destinations                                    | #                         |
          | a    | b  | AB (E1),AB (E1)                                           | ,                                               |                           |
          | c    | d  | CD (Berlin),CD (Berlin)                                   | Berlin,Berlin                                   |                           |
          | e    | f  | EF (A1: Berlin),EF (A1: Berlin)                           | A1: Berlin,A1: Berlin                           |                           |
          | g    | h  | ,                                                         | A1: Berlin,A1: Berlin                           |                           |
          | i    | j  | ,                                                         | Berlin,Berlin                                   |                           |
          | k    | l  | KL (E1),KL (E1)                                           | A1: Berlin,A1: Berlin                           |                           |
          | m    | n  | MN (A1, A2: Berlin, Hamburg),MN (A1, A2: Berlin, Hamburg) | A1, A2: Berlin, Hamburg,A1, A2: Berlin, Hamburg |                           |
          | o    | p  | OP,OP                                                     | ,                                               | guard against mis-tagging |
          | q    | r  | QR (A1, A2),QR (A1, A2)                                   | A1, A2,A1, A2                                   |                           |
