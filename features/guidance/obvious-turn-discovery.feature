@routing  @guidance @obvious
Feature: Simple Turns

    Background:
        Given the profile "car"
        Given a grid size of 10 meters

    #https://www.openstreetmap.org/#map=19/52.51802/13.31337
    Scenario: Crossing a square
        Given the node map
            """
                             d
                             |
            e - - - - - - -  c - g
            |                |
            |                |
            |               |
            |               |
            f - - - - - - - b
                           |
                          a
            """

        And the ways
            | nodes | highway     | name  | oneway |
            | ab    | residential | losch | no     |
            | cd    | residential | ron   | no     |
            | cefb  | residential | alt   | yes    |
            | bc    | residential | alt   | no     |
            | gc    | residential | guer  | no     |

        When I route I should get
            | from | to | route     | turns         |
            | a    | d  | losch,ron | depart,arrive |

    #https://www.openstreetmap.org/#map=18/52.51355/13.21988
    Scenario: Turning tertiary next to residential
        Given the node map
            """
            a - - - b - - - - c
                    |
                     d
                      ` _
                          `-_
                              e
            """

         And the ways
            | nodes | highway     | name  |
            | abde  | tertiary    | havel |
            | bc    | residential | anger |

         When I route I should get
            | from | to | route             | turns                        |
            | a    | c  | havel,anger       | depart,arrive                |
            | a    | e  | havel,havel,havel | depart,continue right,arrive |

     #https://www.openstreetmap.org/#map=19/52.50996/13.23183
     Scenario: Tertiary turning at unclassified
        Given the node map
            """
            a - - - b - - - c
                    |
                    |
                    d
            """

        And the ways
            | nodes | highway      |
            | ab    | tertiary     |
            | bd    | tertiary     |
            | bc    | unclassified |

        When I route I should get
            | from | to | route    | turns                    |
            | a    | c  | ab,bc    | depart,arrive            |
            | a    | d  | ab,bd,bd | depart,turn right,arrive |

    #https://www.openstreetmap.org/#map=19/52.50602/13.25468
    Scenario: Small offset due to large Intersection
        Given the node map
            """
            a - - - - - b - - - - - - c
                        |
                        d
                     `     `
                  `           `
               f                 e
            """

        And the ways
            | nodes | highway     |
            | abc   | residential |
            | bde   | residential |
            | df    | residential |

        When I route I should get
            | from | to | route       | turns                          |
            | a    | e  | abc,bde,bde | depart,turn right,arrive       |
            | a    | f  | abc,df,df   | depart,turn sharp right,arrice |

    # https://www.openstreetmap.org/#map=19/52.49709/13.26620
    # https://www.openstreetmap.org/#map=19/52.49458/13.26273
    Scenario: Offsets in road
        Given the node map
        """
                                              i
                                              |
                                              |
                                              |
        a - - - - - - - b                     e - - - - - - - - - f
                          `c - - - - - - - d`
                           |               |
                           |               |
                           |               |
                           g               h
        """

        And the ways
            | nodes  | highway     | name   |
            | abcdef | residential | Zikade |
            | gc     | residential | Lärche |
            | hd     | residential | Kiefer |
            | ei     | residential | Kiefer |

        When I route I should get
            | from | to | route                | turns                    | locations |
            | a    | f  | Zikade,Zikade        | depart,arrive            | a,f       |
            | f    | a  | Zikade,Zikade        | depart,arrive            | f,a       |
            | a    | g  | Zikade,Lärche,Lärche | depart,turn right,arrive | a,c,g     |
            | h    | i  | Kiefer,Kiefer        | depart,arrive            | h,i       |
            | i    | h  | Kiefer,Kiefer        | depart,arrive            | i,h       |
            | h    | f  | Kiefer,Zikade,Zikade | depart,turn right,arrive | h,d,f     |


     # https://www.openstreetmap.org/#map=20/52.49408/13.27375
     Scenario: Straight on unnamed service
        Given the node map
            """
            a - - - - b - - - - c
                         ` .
                             d
            """

        And the ways
            | nodes | highway | name |
            | abc   | service |      |
            | bd    | service |      |

        When I route I should get
            | from | to | route | turns                           | locations |
            | a    | c  | ,     | depart,arrive                   | a,c       |
            | a    | d  | ,,    | depart,turn slight right,arrive | a,b,d     |

    # https://www.openstreetmap.org/#map=19/52.49198/13.28069
    Scenario: Curved roads at turn
        Given the node map
            """
                               ............g
                            .f
                        e```
                       /
            a  - - - - b
                         c
                           `
                            `
                             `
                               d
            """

        And the ways
            | nodes | highway     | name    |
            | abcd  | residential | herbert |
            | befg  | residential | casper  |

        When I route I should get
            | from | to | route                 | turns                   |
            | a    | d  | herbert,herbert       | depart,arrive           |
            | a    | g  | herbert,casper,casper | depart,turn left,arrive |

    # https://www.openstreetmap.org/#map=19/52.49189/13.28431
    Scenario: Turning residential
        Given the node map
            """
                      d
                     `
            a - - - b
                      \
                        \
                          c
            """

        And the ways
            | nodes | highway     | name    | oneway |
            | abc   | residential | bismark | yes    |
            | bd    | residential | caspar  | yes    |

        When I route I should get
            | from | to | route                 | turns                   |
            | a    | c  | bismark,bismark       | depart,arrive           |
            | a    | d  | bismark,caspar,caspar | depart,turn left,arrive |

    # https://www.openstreetmap.org/#map=19/52.48681/13.28547
    Scenario: Slight Loss in Category with turning road
        Given the node map
            """
                             g
                            /
                           f
                         e   ..c - - - - d
            a - - - - b`````
            """

        And the ways
            | nodes | highway     | name |
            | ab    | tertiary    | warm |
            | bcd   | residential | warm |
            | begg  | tertiary    | paul |

        When I route I should get
            | from | to | route          | turns                          |
            | a    | d  | warm,warm      | depart,arrive                  |
            | a    | g  | warm,paul,paul | depart,turn slight left,arrive |

    # https://www.openstreetmap.org/#map=19/52.48820/13.29947
    Scenario: Driveway within curved road
        Given the node map
            """
            f--e
                 \
                  `
               g   d
                \  |
                  \|
                   c
                 ./
                .b
            a-`
            """

        And the ways
            | nodes | highway     | name    | oneway |
            | abc   | residential | charlot | no     |
            | cdef  | residential | fried   | yes    |
            | gc    | service     |         |        |

        When I route I should get
            | from | to | route         | turns                   |
            | a    | f  | charlot,fried | depart,arrive           |
            | a    | g  | charlot,,     | depart,turn left,arrive |

    # https://www.openstreetmap.org/#map=20/52.46815/13.33984
    Scenario: Curve onto end of the road
        Given the node map
            """
            d - - - - e - f-_
                             ``g
                                 h
                    a - - - - - - b - - _ _ _
                                              ` ` ` c
            """

        And the ways
            | nodes   | highway     | name |
            | ab      | residential | menz |
            | defghbc | residential | rem  |

        When I route I should get
            | from | to | route       | turns                        |
            | a    | c  | menz,rem    | depart,arrive                |
            | d    | c  | rem,rem,rem | depart,continue left,arrive  |
            | c    | d  | rem,rem,rem | depart,continue right,arrive |
            | c    | a  | rem,menz    | depart,arrive                |

    # https://www.openstreetmap.org/#map=19/37.58151/-122.34863
    Scenario: Straight towards oneway street, Service Category, Unnamed
        Given the node map
            """
            a - - b - - c
                  |
                  d
            """

        And the ways
            | nodes | highway | name | oneway |
            | ab    | service |      |        |
            | cb    | service |      | yes    |
            | bd    | service |      |        |

        When I route I should get
            | from | to | route | turns                    |
            | a    | d  | ,     | depart,turn right,arrive |

    # https://www.openstreetmap.org/#map=19/37.61256/-122.40371
    Scenario: Turning Road with Offset at Segregated Intersection
        Given the node map
            """
                   i     h
                   |     |
                   |   . f - - g
            a _    | `   |
               b - c - - d - - e
                   |     |
                   |     |
                   j     k
            """

        And the ways
            | nodes | highway     | name   | oneway |
            | abcd  | residential | park   | no     |
            | cfg   | residential | diego  | no     |
            | de    | service     |        | yes    |
            | kdfh  | primary     | camino | yes    |
            | icj   | primary     | camino | yes    |

        When I route I should get
            | from | to | route              | turns                          |
            | a    | g  | park,diego,diego   | depart,turn slight left,arrive |
            | a    | h  | park,camino,camino | depart,turn left,arrive        |
            | a    | j  | park,camino,camino | depart,turn right,arrive       |

    # https://www.openstreetmap.org/#map=19/37.76407/-122.49642
    Scenario: Road splitting of straight turn
        Given the node map
            """
            d - - - - e - - - - -.f - - - - g
                        .````````
            a - - - - b - - - - - c
            """

        And the ways
            | nodes | highway        | name   | oneway |
            | ab    | secondary      | 37     | yes    |
            | bc    | residential    | 37     | yes    |
            | bf    | secondary_link | Sunset | yes    |
            | defg  | secondary      | Sunset | yes    |

        When I route I should get
            | from | to | route            | turns                          |
            | a    | c  | 37,37            | depart,arrive                  |
            | a    | g  | 37,Sunset,Sunset | depart,turn slight left,arrive |

    # https://www.openstreetmap.org/#map=19/37.77072/-122.41727
    Scenario: Splitting road at traffic island before a turn
        Given the node map
            """
                                              _ _   e
                   .c - - - - - - - - - d - `
            a - - b                   `
                   `f g`           `
                        h       `
                        |    `
                        i  `
                      .`.`
                     j.
                  .
                .
              .
            k
            """

        And the ways
            | nodes  | highway | name   | oneway |
            | ab     | primary | howard | no     |
            | dcb    | primary | howard | yes    |
            | bfghij | primary | howard | yes    |
            | edjk   | primary | ness   | yes    |


        When I route I should get
            | from | to | route              | turns                              | # |
            | a    | k  | howard,ness,ness   | depart,turn left,arrive            |   |
            | e    | k  | ness,ness          | depart,continue slight left,arrive | #if modelled better, a depart/arrive would be more desirable |
            | e    | a  | ness,howard,howard | depart,turn slight right,arrive    |   |

    # https://www.openstreetmap.org/#map=19/37.63171/-122.46205
    Scenario: Weird combination of turning roads
        Given the node map
            """
            a
            |
            b
            |`
            | e
            |  `f
            |     ` g _
            | j-- - - - h - - i
            k
            | l - - - - m - - n
            |      q
            |  p
            | o
            |
            |
            c
            |
            d
            """

        And the ways
            | nodes | highway       | name  | oneway |
            | abk   | tertiary_link | loop  | no     |
            | kcd   | residential   | loop  | no     |
            | copqm | tertiary_link |       | yes    |
            | klmn  | tertiary_link | drive | yes    |
            | ih    | tertiary_link | drive | yes    |
            | hjk   | residential   | drive | yes    |
            | hgfeb | tertiary_link |       | yes    |

        When I route I should get
            | from | to | turns                    | route            |
            | i    | a  | depart,turn right,arrive | drive,loop,loop  |
            | i    | d  | depart,turn left,arrive  | drive,loop,loop  |
            | a    | n  | depart,turn left,arrive  | loop,drive,drive |
            | d    | n  | depart,turn right,arrive | loop,drive,drive |

    # https://www.openstreetmap.org/#map=19/37.26591/-121.84474
    Scenario: Road splitting (unmerged)
        Given the node map
            """
                                      . g - - - h
            d - - - - c - - e  - - f `
                        . ` `
            a - - - - b    `
                           `
                          `
                          `
                         `
                         `
                        i
            """

        And the ways
            | nodes | highway | name | oneway |
            | ei    | service |      |        |
            | abe   | primary | lane | yes    |
            | ecd   | primary | lane | yes    |
            | hgfe  | primary | lane | no     |

        When I route I should get
            | from | to | route     | turns                    |
            | a    | h  | lane,lane | depart,arrive            |
            | a    | i  | lane,,    | depart,turn right,arrive |
            | h    | d  | lane,lane | depart,arrive            |
            | h    | i  | lane,,    | depart,turn left,arrive  |


    # https://www.openstreetmap.org/#map=19/37.85108/-122.27078
    Scenario: Turning Road category
        Given the node map
            """
             e   d
             |   |
             |   |
             f   c    i
             |   |  `
             |   j`
             | `  |   l
             |k   |.`
             |    |
             g    b
             |    |
             |   |
            |    |
            h   a
            """

        And the ways
            | nodes | highway  | name    | oneway |
            | abl   | primary  | adeline | yes    |
            | bjcd  | tertiary | martin  | yes    |
            | efg   | tertiary | martin  | yes    |
            | ijkgh | primary  | adeline | yes    |

        When I route I should get
            | from | to | route                   | turns                               |
            | e    | h  | martin,adeline          | depart,arrive                       |
            | a    | d  | adeline,martin,martin   | depart,turn slight left, arrive     |
            | a    | l  | adeline,adeline,adeline | depart,continue slight right,arrive |
            | i    | h  | adeline,adeline         | depart,arrive                       |

    # https://www.openstreetmap.org/#map=19/37.76471/-122.49639
    Scenario: Turning road
        Given the node map
            """
            f - - - - - - - e - - - - d
                      g
                    h
            a - - - b - - - - - - - - c
                    |
                    |
                    i
            """

        And the ways
            | nodes | highway   | name    | oneway |
            | abd   | primary   | lincoln | yes    |
            | def   | primary   | lincoln | yes    |
            | eghbi | secondary | 37      | yes    |

        When I route I should get
            | from | to | route           | turns                   |
            | d    | i  | lincoln,37,37   | depart,turn left,arrive |
            | d    | f  | lincoln,lincoln | depart,arrive           |

    # https://www.openstreetmap.org/#map=19/37.63541/-122.48343
    # https://www.openstreetmap.org/#map=19/52.47752/13.28864
    Scenario: Road Splitting up
        Given the node map
            """
                                       d
                                      `
                                    `
                                  `
            a - - - - - - - - - b
                                   `
                                       `
                                           `c

            """

        And the ways
            | nodes | highway     | name   |
            | abc   | residential | vista  |
            | bd    | residential | sierra |

        When I route I should get
            | from | to | route               | turns                   |
            | a    | c  | vista,vista         | depart,arrive           |
            | a    | d  | vista,sierra,sierra | depart,turn left,arrive |

    # https://www.openstreetmap.org/#map=19/52.45191/13.44113
    Scenario: Road Splitting up at a Driveway
        Given the node map
            """
                                    d
                                   `
                                  `
                                 `
            a - - - - - - - - - b - - - - - e
                                   `
                                      `
                                        `
                                           `c
            """

        And the ways
            | nodes | highway     | name  |
            | abc   | residential | britz |
            | bd    | residential | palz  |
            | be    | service     |       |

        When I route I should get
            | from | to | route       | turns                       |
            | a    | c  | britz,britz | depart,arrive               |
            | a    | e  | britz,,     | depart,turn straight,arrive |

    # https://www.openstreetmap.org/#map=20/37.62997/-122.49246
    Scenario: Curving road with name-handoff
        Given the node map
            """
                         a
                         |
                         |
                         b
                         `
                          c
                           `
                            d
            l - k         h   ` e
                 ` j - i           ` ` f - - - - - g
            """

        And the ways
            | nodes  | highway     | name  | oneway |
            | abcd   | tertiary    | palm  | no     |
            | defg   | tertiary    | clare | no     |
            | lkjihd | residential | clare | yes    |

        When I route I should get
            | from | to | route            | turns                    |
            | a    | g  | palm,clare,clare | depart,turn left,arrive  |
            | g    | a  | clare,palm,palm  | depart,turn right,arrive |
            | l    | g  | clare,clare      | depart,arrive            |
            | l    | a  | clare,palm,palm  | depart,turn left,arrive  |

    # https://www.openstreetmap.org/#map=19/37.84291/-122.23681
    Scenario: Two roads turning into the same direction
    Given the node map
        """
                  e
                 |
                 |.c
        a - - - b`
                |
                |
                d
        """

    And the ways
        | nodes | highway     | name   | oneway |
        | abc   | residential | romany | no     |
        | db    | residential | ost    | yes    |
        | be    | residential | martin | yes    |

    When I route I should get
        | from | to | route         | turns         |
        | a    | c  | romany,romany | depart,arrive |

    # https://www.openstreetmap.org/#map=19/37.82815/-122.28733
    Scenario: Turning Secondary Next to Residential
    Given the node map
        """
               c
             |   |
             |   |
        -----`   |
        a      b - - - d
        ----------
        """

    And the ways
        | nodes | highway     | name    |
        | ab    | secondary   | mandela |
        | bc    | secondary   | horton  |
        | bd    | residential | yerda   |

    When I route I should get
        | from | to | route                 | turns                       |
        | a    | d  | mandela,yerda,yerda   | depart,turn straight,arrive |
        | a    | c  | mandela,horton,horton | depart,turn left,arrive     |

    # https://www.openstreetmap.org/#map=19/52.46341/13.40272
    Scenario: Loss of road class on turn (segregated)
    Given the node map
        """
        f - - - e - - - d
                |
                1
                |
        a - - - b - - - c
                |
                |
                g
        """

    And the ways
        | nodes | highway     | name | oneway |
        | ab    | tertiary    | germ | yes    |
        | ef    | tertiary    | germ | yes    |
        | de    | tertiary    | ober | yes    |
        | bc    | tertiary    | ober | yes    |
        | be    | tertiary    | germ | no     |
        | bg    | residential | germ | no     |

    When I route I should get
        | from | to | route          | turns                        |
        | 1    | g  | germ,germ      | depart,arrive                |
        | d    | g  | ober,germ,germ | depart,turn left,arrive      |
        | a    | g  | germ,germ,germ | depart,continue right,arrive |

    # https://www.openstreetmap.org/#map=19/37.29821/-121.86874
    Scenario: Sliproads of Higher Category when entering Intersection
    Given the node map
        """
                        e
                        |
                       g|
                        |
                        |
                     h  |
                   i    |
        a - - b - - - c - - d
                   j    |
                     k  |
                        |
                       l|
                        |
                        f
        """

    And the ways
        | nodes | highway      | name  | oneway |
        | abcd  | tertiary     | stone | no     |
        | ecf   | primary      | curt  | yes    |
        | eghib | primary_link |       | yes    |
        | bjklf | primary_link |       | yes    |

    When I route I should get
        | from | to | route       | turns         |
        | a    | d  | stone,stone | depart,arrive |


    # https://www.openstreetmap.org/#map=19/37.63866/-122.46677
    Scenario: Slightly offset traffic circle
    Given the node map
        """
                       i----h
                      `       g
                     `         `
        a - - - - - b          |
                     c         f
                      ` d - e `
        """

    And the ways
        | nodes      | highway     | name |
        | abcdefghib | residential | road |

    When I route I should get
        | from | to | route     | turns         |
        | a    | e  | road,road | depart,arrive |
        | i    | a  | road,road | depart,arrive |
        | d    | a  | road,road | depart,arrive |

    # https://www.openstreetmap.org/#map=19/37.63829/-122.46345
    Scenario: Entering a motorway (curved)
    Given the node map
        """
        a - - - - - - b - - - c - - - - - d
                         e g
                          f
                          |
                      i - h - j
        """

    And the ways
        | nodes | highway     | name  | oneway |
        | abcd  | trunk       | sky   | yes    |
        | befgc | trunk_link  |       | yes    |
        | fh    | trunk_link  |       | no     |
        | ihj   | residential | susan | no     |

    When I route I should get
        | from | to | route      | turns                    |
        | i    | d  | susan,,sky | depart,turn left,arrive  |
        | j    | d  | susan,,sky | depart,turn right,arrive |


    # https://www.openstreetmap.org/edit#map=18/48.99155/8.43520
    Scenario: Passing a motorway link on a trunk road
    Given the node map
        """
        a - - - - - - - - - - b-----
                                `    ```````````` c
                                  `  ` ` ` .
                                             ` d
        """

    And the ways
        | nodes | highway       | name | ref    | oneway |
        | abc   | trunk         | sued | K 9652 | yes    |
        | bd    | motorway_link |      | A 5    | no     |

    When I route I should get
        | from | to | route     | turns                       |
        | a    | c  | sued,sued | depart,arrive               |
        | a    | d  | sued,,    | depart,on ramp right,arrive |

    # https://www.openstreetmap.org/#map=19/49.01098/8.36052
    Scenario: Splitting Road at intersection (modelled turn)
    Given the node map
        """
                    h
                    |
        j - - - - - g - - - - - - - - - - - - i
                    f
                   e
                  d
        a - - - b - - - - - - - - - - - - - - c
        """

    And the ways
        | nodes | highway      | name  | oneway |
        | ab    | primary      | ente  | yes    |
        | bc    | primary      | rhein | yes    |
        | bdefg | primary_link | rhein | yes    |
        | ig    | primary      | rhein | yes    |
        | gj    | primary      | ente  | yes    |
        | gh    | residential  | rhein | no     |

    When I route I should get
        | from | to | route            | turns                           |
        | a    | c  | ente,rhein,rhein | depart,new name straight,arrive |
        | a    | h  | ente,rhein,rhein | depart,turn left,arrive         |
