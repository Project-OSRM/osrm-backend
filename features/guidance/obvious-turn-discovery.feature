@routing  @guidance @obvious
Feature: Simple Turns

  Background:
    Given the profile "car"
    Given a grid size of 10 meters

  # https://www.openstreetmap.org/#map=19/52.51802/13.31337
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


  # https://www.openstreetmap.org/#map=18/52.51355/13.21988
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
  # this scenario detects obvious correctly, but requires changes in collapsing roads
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
        | from | to | route           | turns                                  |
        | a    | e  | abc,bde,bde,bde | depart,turn right,continue left,arrive |
        | a    | f  | abc,bde,df,df   | depart,turn right,turn right,arrive    |

  # https://www.openstreetmap.org/#map=19/52.49709/13.26620
  # https://www.openstreetmap.org/#map=19/52.49458/13.26273
  # scenario requires handling in post-processing
  @todo
  Scenario: Offsets in road
    Given a grid size of 3 meters
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
        | nodes | highway     | name    | oneway |
        | abcd  | residential | herbert | yes    |
        | befg  | residential | casper  | yes    |

    When I route I should get
        | from | to | route                   | turns                        |
        | a    | d  | herbert,herbert,herbert | depart,continue right,arrive |
        | a    | g  | herbert,casper,casper   | depart,turn left,arrive      |

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
        | from | to | route                   | turns                        |
        | a    | c  | bismark,bismark,bismark | depart,continue right,arrive |
        | a    | d  | bismark,caspar,caspar   | depart,turn left,arrive      |

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
        | befg  | tertiary    | paul |

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
             w
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
        | a    | d  | ,,    | depart,turn right,arrive |


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
        | from | to | route                    | turns                                    |
        | a    | g  | park,diego,diego         | depart,turn slight left,arrive           |
        | a    | h  | park,diego,camino,camino | depart,turn slight left,turn left,arrive |
        | a    | j  | park,camino,camino       | depart,turn right,arrive                 |


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
  # requires changes in post-processing
  @todo
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
  # requires changes in post-processing
  @todo
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
        | from | to | route     | turns                          |
        | a    | h  | lane,lane | depart,arrive                  |
        | a    | i  | lane,,    | depart,turn sharp right,arrive |
        | h    | d  | lane,lane | depart,arrive                  |
        | h    | i  | lane,,    | depart,turn left,arrive        |


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
         | `  |
         |k   |   l
         |    |.`
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
        | e    | h  | martin,adeline,adeline  | depart,turn straight,arrive         |
        | a    | d  | adeline,martin,martin   | depart,turn slight left,arrive      |
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
        | from | to | route           | turns                          |
        | d    | i  | lincoln,37,37   | depart,turn slight left,arrive |
        | d    | f  | lincoln,lincoln | depart,arrive                  |

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
                                       `  c

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
  # check service road handling in `is_similar_turn`
  @todo
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
  # test is mutually exclusive with features/guidance/fork.feature:27 "Scenario: Don't Fork On Single Road"
  @todo
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
        | from | to | route            | turns                              |
        | a    | g  | palm,clare,clare | depart,new name slight left,arrive |
        | g    | a  | clare,palm,palm  | depart,turn right,arrive           |
        | l    | g  | clare,clare      | depart,arrive                      |
        | l    | a  | clare,palm,palm  | depart,turn left,arrive            |

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
  # the similarity check considers `bc` as non-similar to `bd` due to a name change "mandela"->"horton"
  # this behavior is captured by features/guidance/turn.feature:126 "Scenario: Three Way Intersection"
  # and is mutually exclusive with the test expectation
  @todo
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
        | from | to | route          | turns                        |
        | a    | e  | road,road,road | depart,continue right,arrive |
        | i    | a  | road,road,road | depart,continue right,arrive |
        | d    | a  | road,road,road | depart,continue left,arrive  |


  # https://www.openstreetmap.org/#map=19/37.63829/-122.46345
  # scenario geometry must be updated to catch the OSM map
  @todo
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


  # https://www.openstreetmap.org/#map=18/48.99155/8.43520
  Scenario: Passing a motorway link on a trunk road
    Given the node map
        """
        a - - - - - - - - - - b-----
                                `    ````````````c
                                   ` d
        """

    And the ways
        | nodes | highway       | name | ref    | oneway |
        | abc   | trunk         | sued | K 9652 | yes    |
        | bd    | motorway_link |      | A 5    | yes    |

    When I route I should get
        | from | to | route     | turns                       |
        | a    | c  | sued,sued | depart,arrive               |
        | a    | d  | sued,,    | depart,off ramp slight right,arrive |


  # https://www.openstreetmap.org/#map=19/48.98900/8.43800
  Scenario: Splitting motorway links without names but with destinations
    Given the node map
        """
                                           .... h
                                     . .g.`
                          ..... e..`............... f
               ... d---```
             c`
            /
          /
         b
        /
        a
        """

    And the ways
        | nodes | highway       | name | destination | oneway |
        | abcde | motorway_link |      |             | yes    |
        | ef    | motorway_link |      | right       | yes    |
        | egh   | motorway_link |      | left        | yes    |

    When I route I should get
        | from | to | route | turns                           |
        | a    | f  | ,,    | depart,fork slight right,arrive |
        | a    | h  | ,,    | depart,fork slight left,arrive  |

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


  # https://www.openstreetmap.org/#map=19/48.99776/8.47766
  Scenario: Narrow cross
    Given the node map
        """
        a       e
         \     /
          \   /
           \ /
            b
           / \
          /   \
         /     \
        d       c
        """

    And the ways
        | nodes | highway   | name   |
        | ab    | secondary | baden  |
        | be    | primary   | gym    |
        | bc    | tertiary  | ritter |
        | db    | primary   | baden  |

    When I route I should get
        | from | to | route             | turns                         |
        | a    | d  | baden,baden,baden | depart,continue right,arrive  |
        | a    | c  | baden,ritter      | depart,arrive                 |
        | a    | e  | baden,gym,gym     | depart,turn sharp left,arrive |


  # https://www.openstreetmap.org/#map=19/48.99870/8.48122
  Scenario: Residential Segregated
    Given the node map
        """
              f     e
              |     |
              |     |
              |     |
              |     |
              |     |
              |     |
              |     |
        a - - b - - c - - d
        """

    And the ways
        | nodes | highway     | name    | oneway |
        | ab    | residential | possel  | no     |
        | fb    | residential | berg    | yes    |
        | bc    | residential | berg    | no     |
        | ce    | residential | berg    | yes    |
        | cd    | residential | kastell | no     |

    When I route I should get
        | from | to | route            | turns                   |
        | a    | d  | possel,kastell   | depart,arrive           |
        | a    | e  | possel,berg,berg | depart,turn left,arrive |


  # https://www.openstreetmap.org/#map=19/48.99810/8.46749
  # the test case depends on geometry: for `da` route both `no_name_change_to_candidate`
  # and `compare_road_deviation_is_distinct` are true so `bc` is considered non-similar
  @todo
  Scenario: Slight End of Road
    Given the node map
        """
                          d
                         /
                        /
        a - - - - - - b - - - - - - - c
        """

    And the ways
        | nodes | highway     | name    |
        | abd   | residential | kanzler |
        | bc    | residential | gartner |

    When I route I should get
        | from | to | route                   | turns                        |
        | a    | c  | kanzler,gartner         | depart,arrive                |
        | a    | d  | kanzler,kanzler,kanzler | depart,continue left,arrive  |
        | d    | a  | kanzler,kanzler,kanzler | depart,continue right,arrive |

  # https://www.openstreetmap.org/#map=18/52.46942/13.33159
  Scenario: Curved crossing
    Given the node map
        """
                                                                    f
                                                                  .`
                                     h                          .
                                      `.                    e `
                                        `.               . `
                                          `.         . `
                                            `.   . `
                                              d
                                        c - `   `.
        a - - - - - - - - - b - - ` ` `           `.
                                                    g
        """

    And the ways
        | nodes | highway     | name  | oneway |
        | abcd  | residential | hand  | no     |
        | def   | residential | hand  | yes    |
        | hdg   | secondary   | schmi | no     |

    When I route I should get
        | from | to | route       | turns         |
        | a    | f  | hand,hand   | depart,arrive |
        | h    | g  | schmi,schmi | depart,arrive |


  # https://www.openstreetmap.org/#map=19/52.56562/13.39109
  # the obviousness check depends on geometry at node `c`
  @todo
  Scenario: Dented Straight
    Given the node map
        """
        a - - - b.            . d - - - - e
                   `        `
                       `c`
                        |
                        |
                        f
        """

    And the ways
        | nodes | highway     | name  |
        | abcde | residential | nord  |
        | cf    | residential | stern |

    When I route I should get
        | from | to | route     | turns         |
        | a    | e  | nord,nord | depart,arrive |
        | e    | a  | nord,nord | depart,arrive |


  # https://www.openstreetmap.org/#map=17/52.57124/13.39892
  Scenario: Primary road curved turn
    Given the node map
        """
        m - - l....
                   `k.
        a - - b - .    ``- - - - p
                   `c . \
                     `. `j - - - o
                       d  \
                        \ i
                        e  \
                        |  h
                        |  |
                        f  g


        """

    And the ways
        | nodes | highway        | name   | oneway | lanes |
        | abc   | primary        | schoen | yes    | 2     |
        | cdef  | primary        | grab   | yes    | 1     |
        | klm   | primary        | schoen | yes    | 2     |
        | ghijk | primary        | grab   | yes    | 1     |
        | cj    | secondary_link | mann   | yes    | 1     |
        | jo    | secondary      | mann   | yes    | 1     |
        | pk    | secondary      | mann   | yes    | 1     |

    When I route I should get
        | from | to | route            | turns                           |
        | a    | f  | schoen,grab,grab | depart,turn slight right,arrive |
        | g    | m  | grab,schoen      | depart,arrive                   |
        | a    | o  | schoen,mann,mann | depart,turn straight,arrive     |


  # https://www.openstreetmap.org/#map=18/52.55374/13.41462
  # scenario for internal intersection collapsing
  @todo
  Scenario: Turn Links as Straights
    Given the node map
        """
               l     k
               |     |
        j - - -i - - hg- - - f
               |  /  |/
              /|   / |
        a - --bc - - d - - - e
               |     |
               m     n
        """

    And the ways
        | nodes | highway      | oneway | name |
        | abc   | primary      | yes    | born |
        | ij    | primary      | yes    | born |
        | cde   | primary      | yes    | wisb |
        | fghi  | primary      | yes    | wisb |
        | li    | primary      | yes    | berl |
        | hk    | primary      | yes    | berl |
        | icm   | primary      | yes    | scho |
        | ndh   | primary      | yes    | scho |
        | bh    | primary_link | yes    |      |
        | gc    | primary_link | yes    |      |

    When I route I should get
        | from | to | route      | turns                   | locations | #                                                    |
        | a    | k  | born,,berl | depart,turn left,arrive | a,b,k     | On improved collapse, this should offer berl on turn |
        | f    | m  | wisb,,scho | depart,turn left,arrive | f,g,m     | On improved collapse, this should offer scho on turn |


  # https://www.openstreetmap.org/#map=19/52.56934/13.40131
  Scenario: Crossing Segregated before a turn
    Given the node map
        """
                         _-d
        f - - - e -- ` `
                |        _-c
        a - - -.b -- ` `
             . `
            g`
        """

    And the ways
        | nodes | highway        | oneway | name |
        | ab    | primary        | yes    | scho |
        | bc    | primary        | yes    | brei |
        | de    | primary        | yes    | brei |
        | ef    | primary        | yes    | scho |
        | gb    | secondary      | no     | woll |
        | be    | secondary_link | no     | woll |

    When I route I should get
        | from | to | route          | turns                             |
        | g    | c  | woll,brei,brei | depart,turn slight right,arrive   |
        | g    | f  | woll,scho,scho | depart,turn sharp left,arrive |
        | a    | c  | scho,brei      | depart,arrive                     |
        | d    | f  | brei,scho      | depart,arrive                     |


  # https://www.openstreetmap.org/#map=19/52.58072/13.42985
  Scenario: Trunk Turning into Motorway
    Given the node map
        """
        a - - - - - b - - -
                      ` .   ` ` ` ` ` ` ` ` c
                          ` - - - - d

        """

    And the ways
        | nodes | highway    | name  | oneway |
        | ab    | trunk      | prenz | yes    |
        | bc    | motorway   |       | yes    |
        | bd    | trunk_link |       | yes    |

    When I route I should get
        | from | to | route   | turns                               |
        | a    | c  | prenz,  | depart,arrive                       |
        | a    | d  | prenz,, | depart,off ramp slight right,arrive |


  # https://www.openstreetmap.org/#map=19/52.57800/13.42900
  Scenario: Splitting Secondary
    Given the node map
        """
                                         . . c
                                  .. ``
        a - - - - - - - b ` ` ` `
                          \
                              \ d - - - - - - - - - e
        """

    And the ways
        | nodes | highway   | name | lanes | oneway |
        | ab    | secondary | dame | 2     | no     |
        | bc    | secondary | pase | 2     | no     |
        | bde   | secondary | feuc | 1     | yes    |

    When I route I should get
        | from | to | route          | turns                           |
        | a    | c  | dame,pase,pase | depart,new name straight,arrive  |
        | a    | e  | dame,feuc,feuc | depart,turn slight right,arrive |

  # https://www.openstreetmap.org/#map=19/52.48468/13.34532
  Scenario: Forking into Tertiary
    Given the node map
        """
                             d
                        . . `
        a - - - - - b - - - - - c
        """

    And the ways
        | nodes | highway  | name  | oneway | lanes |
        | ab    | primary  | kenny | yes    | 4     |
        | bc    | tertiary | kenny | yes    | 2     |
        | bd    | primary  | domi  | yes    | 2     |

    When I route I should get
        | from | to | route             | turns                           |
        | a    | c  | kenny,kenny,kenny | depart,continue straight,arrive |
        | a    | d  | kenny,domi,domi   | depart,turn slight left,arrive  |

  # https://www.openstreetmap.org/#map=18/52.56960/13.43815
  Scenario: Turn onto classified
    Given the node map
        """
                       e  f
                      |   |
                     |    |
        a - - - - - b - - c - - . .
                                    `  ` ` d
        """

    And the ways
        | nodes | highway      | name | oneway |
        | abc   | secondary    | roma | no     |
        | cd    | unclassified | roma | no     |
        | eb    | secondary    | roth | yes    |
        | cf    | secondary    | roth | yes    |

    When I route I should get
        | from | to | route          | turns                   |
        | a    | d  | roma,roma      | depart,arrive           |
        | a    | f  | roma,roth,roth | depart,turn left,arrive |


  # https://www.openstreetmap.org/#map=18/52.50908/13.27312
  Scenario: Merging onto a different street
    Given the node map
        """
        d
          `.`.`.`.`.`.b - - - - c
        a`
        """

    And the ways
        | nodes | highway   | name | oneway |
        | ab    | secondary | masu | yes    |
        | dbc   | primary   | theo | yes    |

    When I route I should get
        | from | to | route          | turns                       | #                        |
        | a    | c  | masu,theo,theo | depart,turn straight,arrive | theo is a through street |
        | d    | c  | theo,theo      | depart,arrive               |                          |

  # https://www.openstreetmap.org/#map=18/52.51299/13.28936
  Scenario: Lanes override road classes
    Given the node map
        """
                e
                |
                |
        a - - - b - - - c
                |
                |
                d
        """

    And the ways
        | nodes | highway       | name | lanes |
        | ab    | primary       | knob | 2     |
        | bc    | unclassified  | knob |       |
        | db    | primary       | soph | 3     |
        | be    | living_street | soph | 3     |

    When I route I should get
        | from | to | route          | turns                   |
        | a    | c  | knob,knob      | depart,arrive           |
        | d    | e  | soph,soph      | depart,arrive           |
        | d    | a  | soph,knob,knob | depart,turn left,arrive |


  # https://www.openstreetmap.org/node/30797565
  Scenario: No turn instruction when turning from unnamed onto unnamed
    Given the node map
      """
      a
      |
      |
      |
      |
      b----------------c
      |
      |
      |
      |
      |
      |
      d
      """

    And the ways
      | nodes | highway    | name | ref   |
      | ab    | trunk_link |      |       |
      | db    | secondary  |      | L 460 |
      | bc    | secondary  |      |       |

    When I route I should get
      | from | to | route | turns                    |
      | d    | c  | ,,    | depart,turn right,arrive |
