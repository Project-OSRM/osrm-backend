@guidance @merge-segregated
Feature: Merge Segregated Roads

    Background:
        Given the profile "car"
        Given a grid size of 3 meters

    #http://www.openstreetmap.org/#map=18/52.49950/13.33916
    @negative
    Scenario: oneway link road
        Given the node map
            """
            f - - - - - - -_-_e - - - - d
                      ...''
            a - - - b'- - - - - - - - - c
            """

        And the ways
            | nodes | name | oneway |
            | abc   | road | yes    |
            | def   | road | yes    |
            | be    | road | yes    |

        When I route I should get
            | waypoints | route     | intersections                                |
            | a,c       | road,road | true:90,true:75 true:90 false:270;true:270   |
            | d,f       | road,road | true:270,false:90 false:255 true:270;true:90 |

    #http://www.openstreetmap.org/#map=18/52.48337/13.36184
    @negative
    Scenario: Square Area - Same Name as road for in/out
        Given the node map
            """
                                  i
                                  |
                                  |
                                  |
                                  g
                                /   \
                              /       \
                            /           \
                          /               \
                        /                   \
            a - - - - c                       e - - - - f
                        \                   /
                          \               /
                            \           /
                              \       /
                                \   /
                                  d
                                  |
                                  |
                                  |
                                  j
            """

        And the ways
            | nodes | name | oneway |
            | ac    | road | no     |
            | ef    | road | no     |
            | cdegc | road | yes    |
            | ig    | top  | no     |
            | jd    | bot  | no     |

        When I route I should get
            | waypoints | route               | intersections                                                                                      |
            | a,f       | road,road,road,road | true:90,false:45 true:135 false:270;true:45 true:180 false:315;true:90 false:225 true:315;true:270 |

    #https://www.openstreetmap.org/#map=19/52.50003/13.33915
    @negative
    Scenario: Short Segment due to different roads
        Given the node map
            """
                                              . d
                                          . '
                                      . '
                                  . '
                              . '
            a - - - - - - - b - - c - - - - - - e
                            .     .
                            .     .
                             .   .
                              . .
                               .
                               f
                               |
                               |
                               |
                               |
                               g
            """

        And the ways
            | nodes | name | oneway |
            | abce  | pass | no     |
            | db    | pass | yes    |
            | fg    | aug  | no     |
            | bfc   | aug  | yes    |

        When I route I should get
            | waypoints | route     | intersections                                                                    |
            | a,e       | pass,pass | true:90,false:60 true:90 true:165 false:270,true:90 false:195 false:270;true:270 |

    @negative
    Scenario: Tripple Merge should not be possible
        Given the node map
            """
                          . f - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - g
                        .
            a - - - - b - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - e
                        '
                          ' c - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - d
            """

        And the ways
            | nodes | name  | oneway |
            | ab    | in    | no     |
            | gfb   | merge | yes    |
            | be    | merge | yes    |
            | dcb   | merge | yes    |

        When I route I should get
            | waypoints | route          | intersections                                         |
            | a,e       | in,merge,merge | true:90;false:60 true:90 false:120 false:270;true:270 |

    Scenario: Tripple Merge should not be possible
        Given the node map
            """
                          . f - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - g
                        .
            a - - - - b - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - e
                        '
                          ' c - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - d
            """

        And the ways
            | nodes | name  | oneway |
            | ab    | in    | no     |
            | gfb   | merge | yes    |
            | eb    | merge | yes    |
            | bcd   | merge | yes    |

        When I route I should get
            | waypoints | route          | intersections                                          |
            | a,d       | in,merge,merge | true:90;false:60 false:90 true:120 false:270;true:270  |

    @negative
    Scenario: Don't accept turn-restrictions
        Given the node map
            """
                          c - - - - - - - - - - - - - - - - - - - - - - - - - - - - - d
                       /                                                                  \
            a - - - b                                                                        g - - h
                       \                                                                  /
                          e - - - - - - - - - - - - - - - - - - - - - - - - - - - - - f
            """

        And the ways
            | nodes  | name | oneway |
            | ab     | road | yes    |
            | befgh  | road | yes    |
            | bcdg   | road | yes    |

        # This is an artificial scenario - not reasonable. It is only to test the merging on turn-restrictions
        And the relations
            | type        | way:from | way:to | node:via | restriction  |
            | restriction | ab       | bcdg   | b        | no_left_turn |

        When I route I should get
            | waypoints | route     | intersections                                                            |
            | a,h       | road,road | true:90,false:60 true:120 false:270,true:90 false:240 false:300;true:270 |

    @negative
    Scenario: Actual Turn into segregated ways
        Given the node map
            """
            a - - - b - < - < - < - < - < - < - < - < - < - < - < c -
                    |                                                 \
                    |                                                 |
                    |                                                 |
                      d                                               |
                       \                                              |
                         \                                            |
                          e > - > - > - > - > - > - > - > - > - > - > f - - - - - - g
            """

        And the ways
            | nodes | name | oneway |
            | ab    | road | no     |
            | fcb   | road | yes    |
            | bdef  | road | yes    |
            | fg    | road | no     |

        When I route I should get
            | waypoints | route     | intersections                                                           |
            | a,g       | road,road | true:90,false:90 true:150 false:270,true:90 false:270 true:345;true:270 |

    Scenario: Merging parallel roads with intermediate bridges
    # https://www.mapillary.com/app/?lat=52.466483333333336&lng=13.431908333333332&z=17&focus=photo&pKey=LWXnKqoGqUNLnG0lofiO0Q
    # http://www.openstreetmap.org/#map=19/52.46750/13.43171
        Given the node map
            """
                f
                |
               .e.
              /   \
             /     \
            g       d
            |       |
            |       |
            |       |
            |       |
            |       |
            |       |
            |       |
            |       |
            h       c
             \     /
              \   /
               \ /
                b
                |
                a
                |
                |
            r - x - s
                |
                |
                y
            """

        And the ways
            | nodes | name           | highway   | oneway | lanes |
            | ab    | Hermannstr     | secondary |        | 2     |
            | bc    | Hermannstr     | secondary | yes    | 2     |
            | cd    | Hermannbruecke | secondary | yes    | 2     |
            | de    | Hermannstr     | secondary | yes    | 2     |
            | ef    | Hermannstr     | secondary |        | 4     |
            | eg    | Hermannstr     | secondary | yes    | 2     |
            | gh    | Hermannbruecke | secondary | yes    | 2     |
            | hb    | Hermannstr     | secondary | yes    | 2     |
            | xa    | Hermannstr     | secondary |        | 4     |
            | yx    | Hermannstr     | secondary |        | 4     |
            | rxs   | Silbersteinstr | tertiary  |        | 1     |

        And the nodes
            | node | highway         |
            | x    | traffic_signals |

        #the intermediate intersections of degree two indicate short segments of new names. At some point, we probably want to get rid of these
        When I route I should get
            | waypoints | turns         | route                 | intersections                                                                       |
            | a,f       | depart,arrive | Hermannstr,Hermannstr | true:0,true:0 false:180,true:0 false:180;true:180                                   |
            | f,a       | depart,arrive | Hermannstr,Hermannstr | true:180,false:0 true:180,false:0 true:180;true:0                                   |
            | y,f       | depart,arrive | Hermannstr,Hermannstr | true:0,true:0 true:90 false:180 true:270,true:0 false:180,true:0 false:180;true:180 |
            | f,y       | depart,arrive | Hermannstr,Hermannstr | true:180,false:0 true:180,false:0 true:180,false:0 true:90 true:180 true:270;true:0 |

    Scenario: Four Way Intersection Double Through Street Segregated
        Given the node map
            """
                                                      q          p
                                                      |          |
                                                      |          |
                                                      |          |
                                                      |          |
                                                      |          |
                                                      |          |
                                                      |          |
                                                      |          |
                                                      |          |
                                                      |          |
                                                      |          |
                                                      |          |
                                                      b          c
                                                       \        /
                                                        \      /
                                                         \    /
            j - - - - - - - - - - - - - - - - - i  .      \  /      , d - - - - - - - - - - - - - - - - - o
                                                      .    \/    .
                                                         > a <
                                                      .    /\   '
                                                   .      /  \     '
            k - - - - - - - - - - - - - - - - - h        /    \       e - - - - - - - - - - - - - - - - - n
                                                        /      \
                                                       /        \
                                                      g          f
                                                      |          |
                                                      |          |
                                                      |          |
                                                      |          |
                                                      |          |
                                                      |          |
                                                      |          |
                                                      |          |
                                                      |          |
                                                      |          |
                                                      |          |
                                                      |          |
                                                      l          m
            """

        And the ways
            | nodes  | highway | oneway | name   | lanes |
            | khaij  | primary | yes    | first  | 4     |
            | odaen  | primary | yes    | first  | 4     |
            | qbacp  | primary | yes    | second | 4     |
            | mfagl  | primary | yes    | second | 4     |

       When I route I should get
            | waypoints | route                | turns                        |
            | f,e       | second,first,first   | depart,turn right,arrive     |
            | f,c       | second,second        | depart,arrive                |
            | f,i       | second,first,first   | depart,turn left,arrive      |
            | f,g       | second,second,second | depart,continue uturn,arrive |
            | d,c       | first,second,second  | depart,turn right,arrive     |
            | d,i       | first,first          | depart,arrive                |
            | d,g       | first,second,second  | depart,turn left,arrive      |
            | d,e       | first,first,first    | depart,continue uturn,arrive |
            | b,i       | second,first,first   | depart,turn right,arrive     |
            | b,g       | second,second        | depart,arrive                |
            | b,e       | second,first,first   | depart,turn left,arrive      |
            | b,c       | second,second,second | depart,continue uturn,arrive |
            | h,g       | first,second,second  | depart,turn right,arrive     |
            | h,e       | first,first          | depart,arrive                |
            | h,c       | first,second,second  | depart,turn left,arrive      |
            | h,i       | first,first,first    | depart,continue uturn,arrive |

    Scenario: Middle Island Over Bridge
        Given the node map
            """
              a
              |
             .b.
            c   h
            |   |
            |   |
            1   2
            |   |
            d   g
             'e'
              |
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
            | waypoints | turns         | route       | intersections 									  |
            | a,f       | depart,arrive | road,road   | true:180,false:0 true:180,false:0 true:180;true:0 |
            | c,f       | depart,arrive | bridge,road | true:180,false:0 true:180;true:0 				  |
            | 1,f       | depart,arrive | bridge,road | true:180,false:0 true:180;true:0                  |
            | f,a       | depart,arrive | road,road   | true:0,true:0 false:180,true:0 false:180;true:180 |
            | g,a       | depart,arrive | bridge,road | true:0,true:0 false:180;true:180                  |
            | 2,a       | depart,arrive | bridge,road | true:0,true:0 false:180;true:180                  |

    @negative
    Scenario: Traffic Circle
        Given the node map
            """
            a - - - - b - - -  e  - - - c - - - - d
                        \              /
                            \      /
                               f
                               |
                               |
                               |
                               g
            """

        And the ways
            | nodes | name   | oneway |
            | ab    | left   | no     |
            | bfceb | circle | yes    |
            | fg    | bottom | no     |
            | cd    | right  | no     |

        When I route I should get
            | waypoints | route                           | intersections                                                                                       |
            | a,d       | left,circle,circle,right,right  | true:90;false:90 true:120 false:270;true:60 true:180 false:300;true:90 false:240 true:270;true:270  |
            | g,d       | bottom,circle,right,right       | true:0;true:60 false:180 false:300;true:90 false:240 true:270;true:270                              |

    Scenario: Middle Island
        Given the node map
            """
              a
              |
              b
            c   h
            |   |
            |   |
            |   |
            |   |
            d   g
              e
              |
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

    Scenario: Traffic Island
        Given the node map
            """
                      f
            a - - b <   > d - - e
                      c
            """

        And the ways
            | nodes | name | oneway |
            | ab    | road | no     |
            | de    | road | no     |
            | bcdfb | road | yes    |

        When I route I should get
            | waypoints | route     | intersections    |
            | a,e       | road,road | true:90;true:270 |

    @negative
    Scenario: Turning Road, Don't remove sliproads
        Given the node map
            """
            h - - - - - g - - - - - - f - - - - - e
                               _   '
                           .
            a - - - - - b - - - - - - c - - - - - d
                        |
                        |
                        |
                        i
            """

        And the ways
            | nodes | name | oneway |
            | ab    | road | yes    |
            | bcd   | road | yes    |
            | efgh  | road | yes    |
            | fb    | road | yes    |
            | bi    | turn | yes    |

        And the relations
            | type        | way:from | way:to | node:via | restriction  |
            | restriction | fb       | bcd    | b        | no_left_turn |

        When I route I should get
            | waypoints | route          | turns                   | intersections                                                                   |
            | a,d       | road,road      | depart,arrive           | true:90,false:60 true:90 true:180 false:270;true:270                            |
            | e,h       | road,road      | depart,arrive           | true:270,false:90 true:240 true:270;true:90                                     |
            | e,i       | road,turn,turn | depart,turn left,arrive | true:270;false:90 true:240 true:270,false:60 false:90 true:180 false:270;true:0 |
     @negative
     Scenario: Meeting Turn Roads
        Given the node map
            """
                        k               l
                        |               |
                        |               |
                        |               |
            h - - - - - g - - - - - - - f - - - - - e
                        |   '        '  |
                        |       x       |
                        |   .        .  |
            a - - - - - b - - - - - - - c - - - - - d
                        |               |
                        |               |
                        |               |
                        i               j
            """

        And the ways
            | nodes | name  | oneway |
            | ab    | horiz | yes    |
            | bc    | horiz | yes    |
            | cd    | horiz | yes    |
            | ef    | horiz | yes    |
            | fg    | horiz | yes    |
            | gh    | horiz | yes    |
            | kg    | vert  | yes    |
            | gb    | vert  | yes    |
            | bi    | vert  | yes    |
            | jc    | vert  | yes    |
            | cf    | vert  | yes    |
            | fl    | vert  | yes    |
            | xg    | horiz | no     |
            | xc    | horiz | no     |
            | xf    | horiz | no     |
            | xb    | horiz | no     |
        And the relations
            | type        | way:from | way:to | node:via | restriction      |
            | restriction | bc       | cf     | c        | no_left_turn     |
            | restriction | fg       | gb     | g        | no_left_turn     |
            | restriction | gb       | bc     | b        | no_left_turn     |
            | restriction | cf       | fg     | f        | no_left_turn     |
            | restriction | xb       | xf     | x        | only_straight_on |
            | restriction | xf       | xb     | x        | only_straight_on |
            | restriction | xg       | xc     | x        | only_straight_on |
            | restriction | xc       | xg     | x        | only_straight_on |

        # the goal here should be not to mention the intersection in the middle at all and also suppress the segregated parts
        When I route I should get
            | waypoints | route            | intersections																																    |
            | a,l       | horiz,vert,vert  | true:90;false:0 true:60 true:90 true:180 false:270,true:60 false:120 false:240 false:300,true:0 false:90 false:180 false:240 true:270;true:180 |
            | a,d       | horiz,horiz      | true:90,false:0 true:60 true:90 true:180 false:270,false:0 true:90 false:180 false:270 true:300;true:270									    |
            | j,h       | vert,horiz,horiz | true:0;true:0 true:90 false:180 false:270 true:300,false:60 false:120 false:240 true:300,false:0 false:90 false:120 true:180 true:270;true:90  |
            | j,l       | vert,vert        | true:0,true:0 true:90 false:180 false:270 true:300,true:0 false:90 false:180 true:240 false:270;true:180									    |
