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


            c   d

                  e

                f
            """

        And the ways
            | nodes | name | highway | oneway |
            | ab    | road | primary | no     |
            | bc    | road | primary | yes    |
            | fdb   | road | primary | yes    |
            | de    | turn | primary | no     |

        When I route I should get
            | waypoints | turns                           | route          |
            | f,a       | depart,arrive                   | road,road      |
            | e,a       | depart,turn slight right,arrive | turn,road,road |

    Scenario: Middle Island
        Given the node map
            """
              a

              b
            c   h




            d   g
              e

              f
            """

        And the ways
            | nodes | name | oneway |
            | ab    | road | no     |
            | ef    | road | no     |
            | bcde  | road | yes    |
            | eghb  | road | yes    |

        When I route I should get
            | waypoints | turns         | route     |
            | a,f       | depart,arrive | road,road |
            | c,f       | depart,arrive | road,road |
            | f,a       | depart,arrive | road,road |
            | g,a       | depart,arrive | road,road |

    Scenario: Middle Island Over Bridge
        Given the node map
            """
              a

              b
            c   h


            1   2

            d   g
              e

              f
            """

        And the ways
            | nodes | name   | oneway |
            | ab    | road   | no     |
            | ef    | road   | no     |
            | bc    | road   | yes    |
            | cd    | bridge | yes    |
            | de    | road   | yes    |
            | eg    | road   | yes    |
            | gh    | bridge | yes    |
            | hb    | road   | yes    |

        When I route I should get
            | waypoints | turns                           | route            |
            | a,f       | depart,arrive                   | road,road        |
            | c,f       | depart,new name straight,arrive | bridge,road,road |
            | 1,f       | depart,new name straight,arrive | bridge,road,road |
            | f,a       | depart,arrive                   | road,road        |
            | g,a       | depart,new name straight,arrive | bridge,road,road |
            | 2,a       | depart,new name straight,arrive | bridge,road,road |

    @negative
    Scenario: Don't Collapse Places:
        Given the node map
            """
                        h
                        g




            a b                   e f




                        c
                        d
            """

        And the ways
            | nodes | name   | oneway |
            | ab    | place  | no     |
            | cd    | bottom | no     |
            | ef    | place  | no     |
            | gh    | top    | no     |
            | bcegb | place  | yes    |

        When I route I should get
            | waypoints | turns                                             | route                      |
            | a,d       | depart,turn right,arrive                          | place,bottom,bottom        |
            | a,f       | depart,continue left,continue right,arrive        | place,place,place,place    |
            | d,f       | depart,turn right,continue right,arrive           | bottom,place,place,place   |
            | d,h       | depart,turn right,continue left,turn right,arrive | bottom,place,place,top,top |
