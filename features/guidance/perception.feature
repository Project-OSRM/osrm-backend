@routing  @guidance @perceived-angles
Feature: Simple Turns

    Background:
        Given the profile "car"
        Given a grid size of 5 meters

    Scenario: Turning into splitting road
        Given the node map
            """
              a
              b
              ^
             / \
            c   d
            |   |\
            |   | e
            |   |
            |   |
            |   |
            |   |
            |   |
            |   |
            g   f
            """

        And the ways
            | nodes | name | highway | oneway |
            | ab    | road | primary | no     |
            | bcg   | road | primary | yes    |
            | fdb   | road | primary | yes    |
            | ed    | turn | primary | yes    |

        When I route I should get
            | waypoints | turns                           | route          | intersections                                |
            | f,a       | depart,arrive                   | road,road      | true:0,true:0 false:150 false:180;true:180   |
            | e,a       | depart,turn slight right,arrive | turn,road,road | true:333;true:0 false:150 false:180;true:180 |

    Scenario: Turning into splitting road
        Given the node map
        """
              a
            g-b
              /\
             /  \
            c   d
            |   |\
            |   | e
            |   |
            |   |
            |   |
            |   |
            |   |
            |   |
            |   |
            h   f
        """

        And the ways
            | nodes | name | highway | oneway |
            | ab    | road | primary | no     |
            | bch   | road | primary | yes    |
            | fdb   | road | primary | yes    |
            | de    | turn | primary | no     |
            | bg    | left | primary | yes    |

        When I route I should get
            | waypoints | turns                                     | route               |
            | f,a       | depart,arrive                             | road,road           |
            | e,a       | depart,turn slight right,arrive           | turn,road,road      |
            | e,g       | depart,turn slight right,turn left,arrive | turn,road,left,left |
            | f,g       | depart,turn left,arrive                   | road,left,left      |
            | f,c       | depart,continue uturn,arrive              | road,road,road      |

    @bug @not-sorted @3179
    Scenario: Adjusting road angles to not be sorted
        Given the node map
            """
                                 g
                                |
                               |
                              |
                             _e - - - - - - - - - f
                           /
            a - - - - -b <
                     i     \ _
                h             c - - - - - - - - - d

            """

        And the ways
            | nodes | name  | oneway |
            | ab    | road  | no     |
            | febcd | road  | yes    |
            | ge    | in    | yes    |
            | eh    | right | yes    |
            | ei    | left  | yes    |

        When I route I should get
            | waypoints | route        | turns                    |
            | g,a       | in,road,road | depart,fork right,arrive |
