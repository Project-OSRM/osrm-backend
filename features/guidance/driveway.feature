@driveway  @guidance
Feature: Driveways intersections

    Background:
        Given the profile "car"
        Given a grid size of 5 meters

    Scenario: Road with a turn to service road
        Given the node map
            """
              a
               ~.
                 b----c----d
                 |
                 e
            """

        And the ways
          | nodes | highway | name    | oneway |
          | abc   | trunk   | first   | yes    |
          | cd    | trunk   | second  | yes    |
          | be    | service | parking | yes    |

       When I route I should get
          | waypoints | route        | turns         | locations |
          | a,d       | first,second | depart,arrive | a,d       |


    Scenario: Turn Instead of Ramp
        Given the node map
            """
              a
               ~.
                 b----c----d
                 |
                 e
            """

        And the ways
          | nodes | highway | name    | oneway |
          | ab    | trunk   |         | yes    |
          | bc    | trunk   |         | yes    |
          | cd    | trunk   | second  | yes    |
          | be    | service | parking | yes    |

       When I route I should get
          | waypoints | route   | turns         | locations |
          | a,d       | ,second | depart,arrive | a,d       |


    Scenario: Road with a turn to service road
        Given the node map
          """
               /-----------------e
          a---b------------------c
               `-----------------d
          """

        And the ways
          | nodes | highway | name | oneway |
          | abc   | trunk   | road | yes    |
          | bd    | service | serv | yes    |
          | be    | service | serv | yes    |

       When I route I should get
          | waypoints | route          | turns                           | locations |
          | a,d       | road,serv,serv | depart,turn slight right,arrive | a,b,d     |
          | a,e       | road,serv,serv | depart,turn slight left,arrive  | a,b,e     |
