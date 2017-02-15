@routing  @guidance @turn-angles
Feature: Simple Turns

    Background:
        Given the profile "car"
        Given a grid size of 1 meters

    Scenario: Offset Turn
        Given the node map
            """
            a - - - - - - - - b - - - - - - - - - c
                              |
                              |
                              |
                              |
                              |
                              |
                              d
                                 '
                                    '
                                       '
                                          '
                                             '
                                                e
                                                   '
                                                      '
                                                         '
                                                            '
                                                              '
                                                                 '
                                                                    '
                                                                       '
                                                                          '
                                                                            '
                                                                              '
                                                                                '
                                                                                  '
                                                                                    '
                                                                                      '
                                                                                        '
                                                                                          f
            """

        And the ways
            | nodes  | highway | name | lanes |
            | abc    | primary | road | 4     |
            | bdef   | primary | turn | 2     |

       When I route I should get
            | waypoints | route          | turns                           |
            | a,c       | road,road      | depart,arrive                   |
            | a,e       | road,turn,turn | depart,turn slight right,arrive |
            | e,a       | turn,road,road | depart,turn slight left,arrive  |
            | e,c       | turn,road,road | depart,turn sharp right,arrive  |

    Scenario: Road Taking a Turn after Intersection
        Given the node map
            """
            a - - - - - - - - b - - - - - - - - - c
                              |
                              |
                              |
                              |
                              |
                              |
                              |
                              |
                              |
                              |
                              |
                              |
                              |
                              |
                              |
                              |
                              |
                              d
                                 '
                                    '
                                       '
                                          '
                                             '
                                                e
                                                   '
                                                      '
                                                         '
                                                            '
                                                              '
                                                                 '
                                                                    '
                                                                       '
                                                                          '
                                                                            '
                                                                              '
                                                                                '
                                                                                  '
                                                                                    '
                                                                                      '
                                                                                        '
                                                                                          f
            """

       And the ways
            | nodes  | highway | name | lanes |
            | abc    | primary | road | 4     |
            | bdef   | primary | turn | 2     |

       When I route I should get
            | waypoints | route          | turns                    |
            | a,c       | road,road      | depart,arrive            |
            | a,e       | road,turn,turn | depart,turn right,arrive |
            | e,a       | turn,road,road | depart,turn left,arrive  |
            | e,c       | turn,road,road | depart,turn right,arrive |

    Scenario: U-Turn Lane
        Given the node map
            """
                                                j
                                                |
                                                |
                                                |
                                                |
                                                |
                                                |
                                                |
            i - - - - - - - - - - - - - - - - - g - h
                                                |
                                                |
                                                |
                                                |
                                                |
                                                |
                                                f
                                                |
                                               /
                                             /
                        d - - - - - - - - e
                      /
                    c
                  /
                |
                |
                |
                |
                |
            a - k - - - - - - - - - - - - - - - - - b
            """

        And the ways
            | nodes   | highway      | name | lanes | oneway |
            | akb     | primary      | road | 4     | yes    |
            | hgi     | primary      | road | 4     | yes    |
            | kcdefg  | primary_link |      | 1     | yes    |
            | gj      | tertiary     | turn | 1     |        |

       When I route I should get
            | waypoints | route          | turns                        |
            | a,b       | road,road      | depart,arrive                |
            | a,i       | road,road,road | depart,continue uturn,arrive |
            | a,j       | road,turn,turn | depart,turn left,arrive      |

    #http://www.openstreetmap.org/#map=19/52.50871/13.26127
    Scenario: Curved Turn
        Given the node map
            """

            f - - - - - - - - - - - e - - - - - - - d
                                      l
                                        k
                                          \
                                           \
                                            j
                                             \
                                              \
                                              i
                                               \
                                                \
                                                h
                                                |
                                                |
                                                |
                                                |
            a - - - - - - - - - - - - - - - - - b - c
                                                |
                                                |
                                                |
                                                |
                                                |
                                                |
                                                |
                                                |
                                                g
            """

        And the ways
            | nodes   | highway      | name | lanes | oneway |
            | abc     | primary      | road | 5     | no     |
            | gb      | secondary    | turn | 3     | yes    |
            | bhijkle | unclassified | turn | 2     | yes    |
            | de      | residential  | road |       | yes    |
            | ef      | residential  | road | 2     | yes    |

       When I route I should get
            | waypoints | route          | turns                        | locations |
            | a,c       | road,road      | depart,arrive                | a,c       |
            | c,a       | road,road      | depart,arrive                | c,a       |
            | g,a       | turn,road,road | depart,turn left,arrive      | g,b,a     |
            | g,c       | turn,road,road | depart,turn right,arrive     | g,b,c     |
            | g,f       | turn,road,road | depart,turn left,arrive      | g,e,f     |
            | c,f       | road,road,road | depart,continue right,arrive | c,b,f     |
            | a,f       | road,road,road | depart,continue uturn,arrive | a,b,f     |

    # http://www.openstreetmap.org/#map=19/52.48753/13.52838
    Scenario: Traffic Circle
        Given the node map
            """
                          .   l  .
                      m             k
                                       .
                   .
                                          j

                n
                                            .

               .
                                            i - - - p

              o

               .                            h

            a - b

                                          g
                  c
                                        .

                      d               f
                         .         .
                              e
            """

        And the ways
            | nodes           | highway     | name | lanes | oneway | junction   |
            | ab              | residential | road | 1     | yes    |            |
            | ip              | residential | road | 1     | yes    |            |
            | bcdefghijklmnob | residential | road | 1     | yes    | roundabout |

       When I route I should get
            | waypoints | route               | turns                                         | intersections                                                            |
            | a,p       | road,road,road      | depart,roundabout turn straight exit-1,arrive | true:90;true:165 false:270 false:345,true:90 false:180 true:345;true:270 |

    Scenario: Splitting Road with many lanes
        Given the node map
            """
                              f - - - - - - - - - - - - - - - - - - - - e
                             '
                            '
                           '
                          '
                         '
            a - - - - - b
                         '
                          '
                           '
                            '
                             '
                              c - - - - - - - - - - - - - - - - - - - - d
            """

        And the ways
            | nodes | highway | name | lanes | oneway |
            | ab    | primary | road | 4     | no     |
            | bcd   | primary | road | 2     | yes    |
            | efb   | primary | road | 2     | yes    |

        When I route I should get
            | waypoints | route     | turns         |
            | a,d       | road,road | depart,arrive |
            | e,a       | road,road | depart,arrive |

    @todo
    # currently the intersections don't match up do to the `merging` process.
    # The intermediate intersection is technically no-turn at all, since the road continues.
    Scenario: Splitting Road with many lanes
        Given the node map
            """
                              f - - - - - - - - - - - - - - - - - - - - e
                             '
                            '
                           '
                          '
                         '
            a - - - - - b
                         '
                          '
                           '
                            '
                             '
                              c - - - - - - - - - - - - - - - - - - - - d
            """

        And the ways
            | nodes | highway | name | lanes | oneway |
            | ab    | primary | road | 4     | no     |
            | bcd   | primary | road | 2     | yes    |
            | efb   | primary | road | 2     | yes    |

        When I route I should get
            | waypoints | route     | turns         | intersections ;  |
            | a,d       | road,road | depart,arrive | true:90;true:270 |
            | e,a       | road,road | depart,arrive | true:270;rue:90  |


    #http://www.openstreetmap.org/#map=19/52.54759/13.43929
    Scenario: Curved Turn At Cross
        Given the node map
            """
                                                          h
                                                          |
                                                          |
                                                          |
                                                          |
                                                          |      . b - - - - - - - - - - - - - - - - - - - - - - - a
                                                          |    .
                                                          |  .
                                                          | c
                                                          |
                                                          |
                                                          |
                                                          d
                                                          |
                                                          |
                                                          |
                                                        e |
                                                       .  |
                                                    .     |
            g - - - - - - - - - - - - - - - - - - f       |
                                                          |
                                                          |
                                                          |
                                                          i
            """

        And the ways
            | nodes | lanes | name  |
            | abcd  | 3     | road  |
            | defg  | 2     | road  |
            | hdi   | 2     | cross |

        When I route I should get
            | waypoints | route           | turns                    |
            | a,g       | road,road       | depart,arrive            |
            | i,a       | cross,road,road | depart,turn right,arrive |
            | i,g       | cross,road,road | depart,turn left,arrive  |
            | h,g       | cross,road,road | depart,turn right,arrive |
            | h,a       | cross,road,road | depart,turn left,arrive  |

    #http://www.openstreetmap.org/#map=19/52.56243/13.32666
    Scenario: Modelled Curve on Segregated Road
        Given the node map
            """
              a                                       f
              |                                   .   e - - - - g
              |                           h  '        |
              |                .    '                 |
              |             i                         |
              |         .                             |
              |                                       |
              |     .                                 |
              |                                       |
              |   j                                   |
              |                                       |
              | '                                     |
              |.                                      |
              |                                       |
            m b - - - - - - - - - - - - - - - - - - - k - - - - l
              |                                       |
              c                                       d
            """

        And the ways
            | nodes  | name | oneway | lanes |
            | abc    | road | yes    | 3     |
            | dkef   | road | yes    | 3     |
            | mbkl   | turn | yes    | 3     |
            | gehijb | turn | yes    | 3     |

        When I route I should get
            | waypoints | route          | turns                        |
            | a,c       | road,road      | depart,arrive                |
            | a,l       | road,turn,turn | depart,turn left,arrive      |
            | a,f       | road,road,road | depart,continue uturn,arrive |
            | d,f       | road,road      | depart,arrive                |
            | d,l       | road,turn,turn | depart,turn right,arrive     |
            | d,c       | road,road,road | depart,continue uturn,arrive |
            | g,l       | turn,turn,turn | depart,continue uturn,arrive |
            | g,c       | turn,road,road | depart,turn left,arrive      |

    #http://www.openstreetmap.org/#map=19/52.50878/13.26085
    Scenario: Curved Turn
        Given the node map
            """
            f - - - - - - - - - e - - - - - - - - - d
                                      k
                                        .
                                          j

                                            .

                                              i

                                               .

                                                h
                                                |
                                                |
                                                |
                                                |
                                                |
                                                |
                                                |
                                                |
            a - - - - - - - - - - - - - - - - - b - c
                                                |
                                                |
                                                |
                                                |
                                                |
                                                |
                                                |
                                                |
                                                g
            """

        And the ways
            | nodes   | highway      | name | lanes | oneway |
            | abc     | primary      | road | 5     | no     |
            | gb      | secondary    | turn | 3     | yes    |
            | bhijke  | unclassified | turn | 2     | yes    |
            | de      | residential  | road |       | yes    |
            | ef      | residential  | road | 2     | yes    |

       When I route I should get
            | waypoints | route          | turns                        | locations | #                                                                                       |
            | g,f       | turn,road,road | depart,turn left,arrive      | g,e,f     | #could offer an additional turn at `e` if you don't detect the turn in between as curve |
            | c,f       | road,road,road | depart,continue right,arrive | c,b,f     |                                                                                         |

    #http://www.openstreetmap.org/search?query=52.479264%2013.295617#map=19/52.47926/13.29562
    Scenario: Splitting Roads with curved split
        Given the node map
            """
                                    f - - - - - - - - - - - - - - - - - e

                                   .

                                 .

                              g
                           .
            a - - - - - b
                           .
                              c

                                 .

                                   .

                                    h - - - - - - - - - - - - - - - - - d
            """

        And the ways
            | nodes | highway | name | lanes | oneway |
            | ab    | primary | road | 4     | no     |
            | bchd  | primary | road | 2     | yes    |
            | efgb  | primary | road | 2     | yes    |

        When I route I should get
            | waypoints | route     | turns         |
            | a,d       | road,road | depart,arrive |
            | e,a       | road,road | depart,arrive |

     #http://www.openstreetmap.org/#map=19/52.51710/13.35462
     Scenario: Merge next to modelled turn
        Given the node map
            """
                                        f - - - - - - - - - - - - - - - e
                                      .

                                    .

                                 .

            a - - - - - - - - b
                               .\
                                .\
                                 .\
                                  g \
                                     \
                                   .   \
                                        c- - - - - - - - - - - - - - -  d
                                    h
                                    |
                                    |
                                    |
                                    |
                                    |
                                    |
                                    |
                                    |
                                    |
                                    |
                                    i
            """

        And the ways
            | nodes | name  | oneway | lanes | highway  |
            | ab    | spree | no     | 6     | tertiary |
            | bcd   | spree | yes    | 3     | tertiary |
            | efb   | spree | yes    | 3     | tertiary |
            | bghi  |       | no     |       | service  |

        When I route I should get
            | waypoints | route       | turns                    |
            | a,d       | spree,spree | depart,arrive            |
            | e,a       | spree,spree | depart,arrive            |
            | a,i       | spree,,     | depart,turn right,arrive |
            | e,i       | spree,,     | depart,turn left,arrive  |

    Scenario: Merge next to modelled turn
        Given the node map
            """
                                        f - - - - - - - - - - - - - - - e
                                      .

                                    .

                                 .

            a - - - - - - - - b
                               .\
                                .\
                                 .\
                                  g \
                                     \
                                   .   \
                                        c- - - - - - - - - - - - - - -  d
                                    h
                                    |
                                    |
                                    |
                                    |
                                    |
                                    |
                                    |
                                    |
                                    |
                                    |
                                    i
            """

        And the ways
            | nodes | name  | oneway | lanes | highway       |
            | ab    | spree | no     | 6     | tertiary      |
            | bcd   | spree | yes    | 3     | tertiary      |
            | efb   | spree | yes    | 3     | tertiary      |
            | bghi  |       | no     |       | living_street |

        When I route I should get
            | waypoints | route       | turns                    |
            | a,d       | spree,spree | depart,arrive            |
            | e,a       | spree,spree | depart,arrive            |
            | a,i       | spree,,     | depart,turn right,arrive |
            | e,i       | spree,,     | depart,turn left,arrive  |

    # http://www.openstreetmap.org/#map=18/52.52147/13.41779
    Scenario: Parking Isle
        Given the node map
            """
            c - - - - - - - - - - - - - - b - - - - - - - - - - - - - - a
                                        .
                                      .
                                     .
                                   .
                                  .
                                .
                               .
                            1 .
                             .
                            .
                           .
                          .
                          j - - - - - - - - - - - - - - - - - - - - - - i
                         .
                         .
                         .
                         .
                        d
                        .
                        .
                         .
                          .
                          e
                           .
                           .
                            .
                            k
                              2
                               .
                                 .

            g - - - - - - - - - - f - - - - - - - - - - - - - - - - - - h
            """

        And the ways
            | nodes  | name  | oneway | lanes | highway       |
            | gfh    | allee | yes    | 3     | primary       |
            | bjdekf |       | yes    |       | service       |
            | ij     |       | no     |       | parking aisle |
            | abc    | allee | yes    | 3     | primary       |

        When I route I should get
            | waypoints | route             | turns                        |
            | a,c       | allee,allee       | depart,arrive                |
            | a,h       | allee,allee,allee | depart,continue uturn,arrive |
            | 1,h       | ,allee,allee      | depart,turn left,arrive      |
            | 2,h       | ,allee,allee      | depart,turn left,arrive      |


    #http://www.openstreetmap.org/#map=18/52.56251/13.32650
    @todo
    Scenario: Curved Turn on Separated Directions
        Given the node map
            """
                e                               d
                f                               c - - - - - - - - - - - j
                |                     l    '    |
                |                   '           |
                |               '               |
                |            '                  |
                |         '                     |
                |     n                         |
                |                               |
                |   '                           |
                |                               |
                | '                             |
                |                               |
                |                               |
                |                               |
                |'                              |
                |                               |
                |                               |
                |                               |
                |                               |
                g                               |
                h - - - - - - - - - - - - - - - b - - - - - - - - - - - o
                |                               |
                |                               |
                |                               |
                |                               |
                |                               |
                |                               |
                i                               a
            """

        And the ways
            | nodes  | name   | oneway | lanes | highway       |
            | jc     | Kapweg | yes    | 3     | primary       |
            | clng   | Kapweg | yes    |       | primary_link  |
            | hbo    | Kapweg | yes    | 2     | primary       |
            | efg    | Kurt   | yes    | 4     | secondary     |
            | gh     | Kurt   | yes    | 2     | primary       |
            | hi     | Kurt   | yes    | 3     | primary       |
            | ab     | Kurt   | yes    | 4     | primary       |
            | cd     | Kurt   | yes    | 3     | secondary     |
            | bc     | Kurt   | yes    | 2     | primary       |

        When I route I should get
            | waypoints | route                | turns                        |
            | j,i       | Kapweg,Kurt,Kurt     | depart,turn left,arrive      |
            | j,o       | Kapweg,Kapweg,Kapweg | depart,continue uturn,arrive |
            | a,i       | Kurt,Kurt,Kurt       | depart,continue uturn,arrive |

    #http://www.openstreetmap.org/#map=18/52.56251/13.32650
    Scenario: Curved Turn on Separated Directions
        Given the node map
            """
                e                               d
                f                               c - - - - - - - - - - - j
                |                     l    '    |
                |                   '           |
                |               '               |
                |            '                  |
                |         '                     |
                |     n                         |
                |                               |
                |   '                           |
                |                               |
                | '                             |
                |                               |
                |                               |
                |                               |
                |'                              |
                |                               |
                |                               |
                |                               |
                |                               |
                g                               |
                h - - - - - - - - - - - - - - - b - - - - - - - - - - - o
                |                               |
                |                               |
                |                               |
                |                               |
                |                               |
                |                               |
                i                               a
            """

        And the ways
            | nodes  | name   | oneway | lanes | highway       |
            | jc     | Kapweg | yes    | 3     | primary       |
            | clngh  | Kapweg | yes    |       | primary_link  |
            | hbo    | Kapweg | yes    | 2     | primary       |
            | efh    | Kurt   | yes    | 4     | secondary     |
            | hi     | Kurt   | yes    | 3     | primary       |
            | ab     | Kurt   | yes    | 4     | primary       |
            | cd     | Kurt   | yes    | 3     | secondary     |
            | bc     | Kurt   | yes    | 2     | primary       |

        When I route I should get
            | waypoints | route                | turns                        |
            | j,i       | Kapweg,Kurt,Kurt     | depart,turn left,arrive      |
            | j,o       | Kapweg,Kapweg,Kapweg | depart,continue uturn,arrive |
            | a,i       | Kurt,Kurt,Kurt       | depart,continue uturn,arrive |

    #http://www.openstreetmap.org/#map=19/52.53731/13.36033
    Scenario: Splitting Road to Left
        Given the node map
            """
                                k

                                   .

                                     .

                                      j

                                       .

                                        .

                                         .

            a - - - - - - - b - - - - - - c - - - - - - - d - - - - - - e
                                           .
                                       '
                                    f        .
                                  .
                                              i
                                .            .

                               .            .

                             .            .

                           .             .

                         .             .

                       .             .

                      .             .

                     .             .

                    .             .

                   .             .

                  .             .

                 .             .

                .             .

               .             .

              .             .

             .             .

            .             .

            g             h
            """

        And the nodes
            | node | highway         |
            | i    | traffic_signals |

        And the ways
            | nodes  | name   | oneway | lanes | highway       |
            | abc    | Perle  | no     | 4     | secondary     |
            | cde    | Perle  | no     | 6     | secondary     |
            | cfg    | Heide  | yes    | 2     | secondary     |
            | hic    | Heide  | yes    | 3     | secondary     |
            | cjk    | Friede | no     |       | tertiary      |

        When I route I should get
            | waypoints | route               | turns                    | intersections                                         |
            | a,g       | Perle,Heide,Heide   | depart,turn right,arrive | true:90;true:90 true:180 false:270 true:345;true:18   |
            | a,k       | Perle,Friede,Friede | depart,turn left,arrive  | true:90;true:90 true:180 false:270 true:345;true:153  |
            | a,e       | Perle,Perle         | depart,arrive            | true:90,true:90 true:180 false:270 true:345;true:270  |
            | e,k       | Perle,Friede,Friede | depart,turn right,arrive | true:270;false:90 true:180 true:270 true:345;true:153 |
            | e,g       | Perle,Heide,Heide   | depart,turn left,arrive  | true:270;false:90 true:180 true:270 true:345;true:18  |
            | h,k       | Heide,Friede        | depart,arrive            | true:16,true:90 true:180 true:270 true:345;true:153   |
            | h,e       | Heide,Perle,Perle   | depart,turn right,arrive | true:16;true:90 true:180 true:270 true:345;true:270   |
            | h,a       | Heide,Perle,Perle   | depart,turn left,arrive  | true:16;true:90 true:180 true:270 true:345;true:90    |

    #http://www.openstreetmap.org/#map=19/52.53293/13.32956
    Scenario: Curved Exit from Curved Road
        Given the node map
            """
                                                                        g
                                                                      .
                                                                    .
                                                                  f
                                                                .
                                                             .
                                                           .
                                                        .
                                                     .
                                                  e
                                               .

                                          d
                                       '
                                  c         \
            a - - - - - - b    '
                                              h  .
                                                    i
                                                         '     . .  .   j
            """

        And the ways
            | nodes  | name    | oneway | lanes | highway     |
            | abcd   | Siemens | no     | 5     | secondary   |
            | defg   | Erna    | no     | 3     | secondary   |
            | dhij   | Siemens | no     |       | residential |

        When I route I should get
            | waypoints | route                   | turns                               |
            | a,j       | Siemens,Siemens,Siemens | depart,continue slight right,arrive |
            | a,g       | Siemens,Erna            | depart,arrive                       |
            | g,j       | Erna,Siemens,Siemens    | depart,turn left,arrive             |
            | g,a       | Erna,Siemens            | depart,arrive                       |

     #http://www.openstreetmap.org/#map=19/52.51303/13.32170
     Scenario: Ernst Reuter Platz
        Given the node map
            """
                  j                                     g
                    .                                   .
                       .                               .
                          .                          .
                            i                     h
                               .' e - - k - - d '.
                           '                         .
                        f                               c
                                                           .
                                                              .
                                                                .
                                                                  b
                                                                   .
                                                                     .
                                                                      .
                                                                       .
                                                                        a
            """

        And the ways
            | nodes | name  | oneway | lanes | highway   |
            | abcdk | ernst | yes    | 4     | primary   |
            | ek    | ernst | yes    | 5     | primary   |
            | kef   | ernst | yes    | 4     | primary   |
            | ghd   | march | yes    | 3     | secondary |
            | eij   | otto  | yes    | 2     | secondary |

        When I route I should get
            | waypoints | route           | turns                           |
            | a,j       | ernst,otto,otto | depart,turn slight right,arrive |
            | a,f       | ernst,ernst     | depart,arrive                   |

    #http://www.openstreetmap.org/#map=19/52.52409/13.36829
    Scenario: Tiny Curve at Turn
        Given the node map
            """
                                                h
                                                |
                                                |
                                                |
                                                |
                                                |
                                                |
                                                f - - - - - - - - i - - g
                                                e                 |
                                           .  d                   |
            a - - - - - - - - - - b - c  '                        j
            """

        And the ways
            | nodes  | name  | oneway | lanes | highway     |
            | abcdef | ella  | no     |       | residential |
            | fig    | ella  | no     |       | residential |
            | hf     | ilse  | no     |       | residential |
            | ij     | wash  | no     |       | residential |

        When I route I should get
            | waypoints | route          | turns                    |
            | a,g       | ella,ella      | depart,arrive            |
            | g,a       | ella,ella      | depart,arrive            |
            | g,h       | ella,ilse,ilse | depart,turn right,arrive |
            | a,h       | ella,ilse,ilse | depart,turn left,arrive  |
            | h,g       | ilse,ella,ella | depart,turn left,arrive  |
            | h,a       | ilse,ella,ella | depart,turn right,arrive |

    #http://www.openstreetmap.org/#map=18/52.53738/13.36027
    Scenario: Merging at Turn - Don't report u-turn
        Given the node map
            """
                                                            g
                                                          .
                                                        .
                                                      .
                                                    .
                                                  f
                            h                   .
                               .              .
                                  .         j
                                     .    .
                                        c
                                      . . .
                                    .   .   .
                                  .    .      .
                                .     .         .
                              .      .            .
                            .       .               .
                          .         .               b
                        .          .                 .
                      .            .                 .
                    .             .                  .
                  .               d                  .
                .                 .                  .
              .                    .                 .
            i                      .                 .
                                   .                 .
                                   .                 .
                                   .                 .
                                   .                 .
                                   .                 .
                                   .                 .
                                   .                 .
                                   .                 .
                                   .                 .
                                   .                 .
                                   .                 .
                                   .                 .
                                   .                 .
                                   .                 .
                                   .                 .
                                   .                 .
                                   .                 .
                                   .                 .
                                   .                 .
                                   .                 .
                                   .                 .
                                   .                 .
                                   .                 .
                                   .                 .
                                   .                 .
                                   .                 .
                                   .                 .
                                   .                 .
                                   .                 .
                                   .                 .
                                    e                 a
            """

        And the ways
            | nodes  | name   | oneway | lanes | highway   |
            | abc    | Heide  | yes    | 3     | secondary |
            | cde    | Heide  | yes    | 2     | secondary |
            | cjf    | Perle  | yes    | 6     | secondary |
            | cjf    | Perle  | no     | 6     | secondary |
            | fg     | Fenn   | no     | 6     | secondary |
            | ch     | Friede | no     |       | tertiary  |
            | ic     | Perle  | no     | 4     | secondary |

        And the nodes
            | node | highway         |
            | j    | traffic_signals |
            | b    | traffic_signals |

        When I route I should get
            | waypoints | route               | turns                          |
            | a,e       | Heide,Heide,Heide   | depart,continue uturn,arrive   |
            | a,g       | Heide,Fenn,Fenn     | depart,turn right,arrive       |
            | a,h       | Heide,Friede,Friede | depart,turn slight left,arrive |
            | i,e       | Perle,Heide,Heide   | depart,turn right,arrive       |
            | i,h       | Perle,Friede,Friede | depart,turn left,arrive        |

    #http://www.openstreetmap.org/#map=19/52.48630/13.36017
    Scenario: Don't Break U-turns
        Given the node map
            """
                                          .  . .  b - - - - - - - - - - a
                                  c   '
                           .  ' /
                   .    '     i
            d  '             /
                            /
                            j
                            |
                            |
                            |
                            |
                            |
                            |
            e - - - - - - - k
                            |   .
                            f        .
                            l             '  -  g - - - - - - - - - - - h
            """

        And the ways
            | nodes  | name    | oneway | lanes | highway     |
            | ab     | julius  | yes    | 2     | secondary   |
            | gh     | julius  | yes    | 2     | secondary   |
            | bcd    | kolonne | yes    | 2     | secondary   |
            | ekg    | kolonne | yes    | 2     | secondary   |
            | cijkf  | feurig  | yes    |       | residential |
            | fl     | feurig  | no     |       | residential |

        When I route I should get
            | waypoints | route                   | turns                        |
            | b,g       | kolonne,kolonne,kolonne | depart,continue uturn,arrive |

    #http://www.openstreetmap.org/#map=19/52.51633/13.42077
    Scenario: Service Road at the end with slight offset
        Given the node map
            """
                                          d - - - - - - - - - - - - - - e
                                          |
            c - - - - - - - - - - - - - - b
                                          |
                                          |
                                          |
                                          |
                                          |
                                          |
                                          |
                                          |
                                          |
                                          a
            """

        And the ways
            | nodes  | name | highway     |
            | ab     | holz | residential |
            | bc     |      | service     |
            | bde    |      | service     |

        When I route I should get
            | waypoints | route      | turns                    |
            | e,c       | ,          | depart,arrive            |
            | e,a       | ,holz,holz | depart,turn left,arrive  |
            | c,e       | ,          | depart,arrive            |
            | c,a       | ,holz,holz | depart,turn right,arrive |
            | a,c       | holz,,     | depart,turn left,arrive  |
            | a,e       | holz,,     | depart,turn right,arrive |

    #http://www.openstreetmap.org/#map=19/52.45037/13.51923
    Scenario: Special Turn Road
        Given the node map
            """
            h - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - g - - - - f
                                                                                                      |
                                                                                                      |
                                                                                                      |
                                                                                                      |
                                                                                                      |
                                    d - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - e
                                 .
                              .
                           .
                        .
                     .
            a - - - b - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - c
            """

        And the ways
            | nodes | name | ref | lanes | highway      | oneway |
            | abc   | mich | b9  | 3     | primary      | yes    |
            | fgh   | mich | b9  | 3     | primary      | yes    |
            | bdeg  | mich |     |       | primary_link | yes    |

        When I route I should get
            | waypoints | route          | ref      | turns                        |
            | a,h       | mich,mich,mich | b9,b9,b9 | depart,continue uturn,arrive |

    #http://www.openstreetmap.org/#map=19/52.49449/13.18116
    Scenario: Curved Parking Lot Road
        Given the node map
            """
                                          m
                                          |
                                          |
                                          |
                                          |
            f - - - - - - - - - - - - - - e - - - - - - - - - - - - - - d
                                              l
                                                k
                                                 \
                                                  j
                                                  i
                                                 /
                                                h
                                              g
            a - - - - - - - - - - - - - - b - - - - - - - - - - - - - - c
            """

        And the ways
            | nodes    | name | oneway | highway     |
            | abc      | gato | yes    | residential |
            | def      | gato | yes    | residential |
            | bghijkle |      | yes    | service     |
            | em       | hain | no     | service     |

        When I route I should get
            | waypoints | route          | turns                        |
            | a,f       | gato,gato,gato | depart,continue uturn,arrive |
            | d,m       | gato,hain,hain | depart,turn right,arrive     |
            | a,m       | gato,hain,hain | depart,turn left,arrive      |
            | d,f       | gato,gato      | depart,arrive                |

    #http://www.openstreetmap.org/#map=19/52.60831/13.42990
    Scenario: Double Curve Turn
        Given the node map
            """

                                                              _   .   d
            f - - - - - - - - - - - - - - - - - - - -_- e  '
                                                  .
                                              g
                                             .
                                           .
                                          .
                                         .
                                        h
                                        .                            _  c
                                        .                        .
                                        .                  .
                                        i             .
                                         .       .
                                          .  .
            a - - - - - - - - - - - - - - b
                                             .
                                                 .
                                                      .
                                                          j
                                                               .
                                                                    .
                                                                        k
            """

        And the ways
            | nodes  | name  | oneway | highway   | lanes |
            | ab     | rose  | yes    | secondary | 2     |
            | ef     | rose  | yes    | secondary | 2     |
            | de     | trift | yes    | secondary | 2     |
            | bc     | trift | yes    | secondary | 2     |
            | eghibj | muhle | yes    | tertiary  | 1     |

        And the nodes
            | node | highway         |
            | h    | traffic_signals |
            | j    | traffic_signals |

        When I route I should get
            | waypoints | route             | turns                           |
            | a,c       | rose,trift        | depart,arrive                   |
            | a,k       | rose,muhle,muhle  | depart,turn slight right,arrive |
            | d,f       | trift,rose        | depart,arrive                   |
            | d,k       | trift,muhle,muhle | depart,turn sharp left,arrive   |
            | d,c       | trift,trift,trift | depart,continue uturn,arrive    |

    #http://www.openstreetmap.org/#map=19/52.50740/13.44824
    Scenario: Turning Loop at the end of the road
        Given the node map
            """
                                                      l - - k  _
                                                    .             j
                                                  m                 '
                                                 .                    i
                                                .                      .
                                                .                       .
                                                .                       h
                                                .                       .
                                                n                       .
                                                .                       .
                                                .                       g
                                                o                       .
                                               .                       .
                                              p                       f
                                           .                         .
                                       .                            .
                                   .                            . e
            a - - - - b - - - - c - - - - - - - - - - - - - d '
            """

     And the ways
            | nodes           | name    | highway     | lanes | oneway |
            | abc             | circled | residential | 1     | no     |
            | cdefghijklmnopc | circled | residential | 1     | yes    |

     When I route I should get
            | waypoints | bearings     | route           | turns         |
            | b,a       | 90,2 180,180 | circled,circled | depart,arrive |

    Scenario: Curved Parking Lot Road
        Given the node map
            """
                                          m
                                          |
                                          |
                                          |
                                          |
            f - - - - - - - - - - - - - - e - - - - - - - - - - - - - - d
                                              l
                                                k

                                                  j
                                                  i

                                                h
                                              g
            a - - - - - - - - - - - - - - b - - - - - - - - - - - - - - c
            """

        And the ways
            | nodes    | name | oneway | highway     |
            | abc      | gato | yes    | residential |
            | def      | gato | yes    | residential |
            | bghijkle |      | yes    | service     |
            | em       | hain | no     | service     |

        When I route I should get
            | waypoints | route          | turns                        |
            | a,m       | gato,hain,hain | depart,turn left,arrive      |

    Scenario: Segfaulting Regression
        Given the node map
            """
            a - - - - - - - - - - - - - - b c
                                            |
                                            |
                                            |
                                            d--------------e
            """

        And the ways
            | nodes | lanes:forward |
            | ab    |               |
            | bcde  | 6             |

        When I route I should get
            | waypoints | route   |
            | a,e       | ab,bcde |


    @3401
    Scenario: Curve With Duplicated Coordinates
        Given the node locations
            | node | lat                | lon                | #          |
            | a    | 0.9999280745650984 | 1.0                |            |
            | b    | 0.9999280745650984 | 1.0000179813587253 |            |
            | c    | 0.9999280745650984 | 1.0000359627174509 |            |
            | d    | 0.9999460559238238 | 1.0000674300952204 |            |
            | e    | 0.9999640372825492 | 1.0000809161142643 |            |
            | f    | 0.9999820186412746 | 1.0000854114539457 |            |
            | g    | 1.0                | 1.0000854114539457 |            |
            | h    | 1.0                | 1.0000854114539457 | #same as g |
            | z    | 0.9999100932063729 | 1.0000179813587253 |            |
           #                   g
           #                   |
           #                   f
           #                   '
           #                  e
           #                 '
           #               d
           #           '
           #a - b - c
           #    |
           #    z

        And the ways
            | nodes   | oneway | lanes | #                        |
            | ab      | yes    | 1     |                          |
            | zb      | yes    | 1     |                          |
            | bcdefgh | yes    | 1     | #intentional duplication |

        # we don't care for turn instructions, this is a coordinate extraction bug check
        When I route I should get
            | waypoints | route      | intersections                                |
            | a,g       | ab,bcdefgh | true:90,true:45 false:180 false:270;true:180 |

    #https://github.com/Project-OSRM/osrm-backend/pull/3469#issuecomment-270806580
    Scenario: Oszillating Lower Priority Road
		#Given the node map
	#		"""
	#		a -db    c
    #           f
    #   	"""
        Given the node locations
            | node | lat                | lon                | #          |
            | a    | 1.0                | 1.0                |            |
            | b    | 1.0000179813587253 | 1.0                |            |
            | c    | 1.0000204580571323 | 1.0                |            |
            | d    | 1.0000179813587253 | 1.0                | same as b  |
            | f    | 1.0000179813587253 | 1.0000179813587253 |            |

        And the ways
            | nodes | oneway | lanes | highway |
            | ab    | yes    | 1     | primary |
            | bf    | yes    | 1     | primary |
            | bcd   | yes    | 1     | service |

        # we don't care for turn instructions, this is a coordinate extraction bug check
        When I route I should get
            | waypoints | route |
            | a,d       | ab,ab |

    Scenario: Sharp Turn Onto A Bridge
        Given the node map
            """
              e
              |
              |
              |
              |
              |
              |
              |
              |
              |
              |
              |
              |
              |
             ga - - -b
              f    /
              d -c
            """

        And the ways
            | nodes | oneway | lanes |
            | gaf   | yes    | 1     |
            | abcde | yes    | 1     |

        When I route I should get
            | waypoints | route       |
            | g,e       | abcde,abcde |
