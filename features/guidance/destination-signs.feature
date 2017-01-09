@routing @guidance
Feature: Destination Signs

    Background:
        Given the profile "car"

    Scenario: Car - route name assembly with destination signs
        Given the node map
          """
          a b
          c d
          e f
          g h
          i j
          k l
          m n
          o p
          q r
          s t
          u v
          """

        And the ways
          | nodes | name | ref | destination    | destination:ref | destination:street | oneway | #                                    |
          | ab    | AB   | E1  |                |                 |                    | yes    |                                      |
          | cd    | CD   |     | Berlin         |                 |                    | yes    |                                      |
          | ef    | EF   |     | Berlin         | A1              |                    | yes    |                                      |
          | gh    |      |     | Berlin         | A1              |                    | yes    |                                      |
          | ij    |      |     | Berlin         |                 |                    | yes    |                                      |
          | kl    | KL   | E1  | Berlin         | A1              |                    | yes    |                                      |
          | mn    | MN   |     | Berlin;Hamburg | A1;A2           |                    | yes    |                                      |
          | op    | OP   |     | Berlin;Hamburg | A1;A2           |                    | no     | mis-tagged destination: not a oneway |
          | qr    | QR   |     |                | A1;A2           |                    | yes    |                                      |
          | st    | ST   |     |                |                 | St Street          | yes    |                                      |
          | uv    | UV   |     | Berlin         |                 | St Street          | yes    |                                      |

        When I route I should get
          | from | to | route | destinations                                    | ref   | #                          |
          | a    | b  | AB,AB | ,                                               | E1,E1 |                            |
          | c    | d  | CD,CD | Berlin,Berlin                                   | ,     |                            |
          | e    | f  | EF,EF | A1: Berlin,A1: Berlin                           | ,     |                            |
          | g    | h  | ,     | A1: Berlin,A1: Berlin                           | ,     |                            |
          | i    | j  | ,     | Berlin,Berlin                                   | ,     |                            |
          | k    | l  | KL,KL | A1: Berlin,A1: Berlin                           | E1,E1 |                            |
          | m    | n  | MN,MN | A1, A2: Berlin, Hamburg,A1, A2: Berlin, Hamburg | ,     |                            |
          | o    | p  | OP,OP | ,                                               | ,     | guard against mis-tagging  |
          | q    | r  | QR,QR | A1, A2,A1, A2                                   | ,     |                            |
          | s    | t  | ST,ST | St Street,St Street                             | ,     |                            |
          | u    | v  | UV,UV | Berlin,Berlin                                   | ,     | city preferred over street |
