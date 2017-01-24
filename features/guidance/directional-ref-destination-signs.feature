@routing @guidance
Feature: Destination Signs

    Background:
        Given the profile "car"

    Scenario: Car - route name assembly with destination signs accounting for directional:ref tags
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
          | nodes | name | destination    | destination:ref | destination:ref:forward | destination:ref:backward | destination:forward | destination:backward | oneway | #                                    |
          | ab    | AB   | Berlin         | A1              | A1                      | A2                       |                     |                      | yes    |                                      |
          | cd    | CD   |                | A1              | A1                      | A2                       | Berlin              |  Hamburg             | -1     |                                      |
          | ef    | EF   |                |                 | A1                      | A2                       | Berlin              |  Hamburg             | yes    |                                      |
          | gh    | GH   |                |                 | A1                      | A2                       | Berlin              |  Hamburg             | -1     |                                      |
          | ij    | IJ   | Berlin         | A1              |                         | A2                       | Berlin              |  Hamburg             | yes    |                                      |
          | kl    | KL   |                | A1              |                         | A2                       | Berlin              |  Hamburg             | -1     |                                      |
          | mn    | MN   | Berlin         | A1              | A1                      |                          | Berlin              |  Hamburg             | no     | mis-tagged destination: not a oneway |
         
        When I route I should get
          | from | to | route                                                     | destinations                                    | ref   | #                         |
          | a    | b  | AB,AB                                                     | A1: Berlin,A1: Berlin                           | ,     |                           |
          | d    | c  | CD,CD                                                     | A2: Hamburg,A2: Hamburg                         | ,     |                           |
          | e    | f  | EF,EF                                                     | A1: Berlin,A1: Berlin                           | ,     |                           |
          | h    | g  | GH,GH                                                     | A2: Hamburg,A2: Hamburg                         | ,     |                           |
          | i    | j  | IJ,IJ                                                     | A1: Berlin,A1: Berlin                           | ,     |                           |
          | l    | k  | KL,KL                                                     | A2: Hamburg,A2: Hamburg                         | ,     |                           |
          | m    | n  | MN,MN                                                     | ,                                               | ,     | guard against mis-tagging |
