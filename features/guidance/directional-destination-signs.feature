@routing @guidance
Feature: Destination Signs

    Background:
        Given the profile "car"

    Scenario: Car - route name assembly with destination signs accounting for directional tags
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
          """

        And the ways
          | nodes | name | ref | destination    | destination:ref | destination:forward | destination:backward | oneway | #                                    |
          | ab    | AB   |     | Berlin         |                 |                     |                      | yes    |                                      |
          | cd    | CD   |     |                |                 | Berlin              |                      | yes    |                                      |
          | ef    | EF   |     |                |                 | Berlin              |  Hamburg             | -1     |                                      |
          | gh    | GH   |     |                | A1              |                     |                      | yes    |                                      |
          | ij    | IJ   |     | Berlin         | A1              |                     |                      | no     | mis-tagged destination: not a oneway |
          | kl    | KL   |     |                | A1              | Berlin              |  Hamburg             | yes    |                                      |
          | mn    | MN   |     | Berlin         | A1              | Berlin              |  Hamburg             | yes    |                                      |
          | op    | OP   |     | Berlin         |                 |                     |  Hamburg             | -1     |                                      |
          | qr    | QR   |     |                |                 |                     |  Hamburg             | -1     |                                      |

        When I route I should get
          | from | to | route                                                     | destinations                                    | ref   | # | locations |
          | a    | b  | AB,AB                                                     | Berlin,Berlin                                   | ,     |   | a,b       |
          | c    | d  | CD,CD                                                     | Berlin,Berlin                                   | ,     |   | c,d       |
          | f    | e  | EF,EF                                                     | Hamburg,Hamburg                                 | ,     |   | f,e       |
          | g    | h  | GH,GH                                                     | A1,A1                                           | ,     |   | g,h       |
          | i    | j  | IJ,IJ                                                     | ,                                               | ,     | guard against mis-tagging | i,j       |
          | k    | l  | KL,KL                                                     | A1: Berlin,A1: Berlin                           | ,     |   | k,l       |
          | m    | n  | MN,MN                                                     | A1: Berlin,A1: Berlin                           | ,     |   | m,n       |
          | p    | o  | OP,OP                                                     | Hamburg,Hamburg                                 | ,     |   | p,o       |
          | r    | q  | QR,QR                                                     | Hamburg,Hamburg                                 | ,     |   | r,q       |
