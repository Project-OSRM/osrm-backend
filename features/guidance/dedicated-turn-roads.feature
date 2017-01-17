@routing  @guidance
Feature: Slipways and Dedicated Turn Lanes

    Background:
        Given the profile "car"
        Given a grid size of 5 meters

    Scenario: Turn Instead of Ramp
        Given the node map
            """
                    e
            a b-----c-d
               `--h |
                   ||
                  1||
                   ||
                   `f
                    |
                    g
            """

        And the ways
            | nodes | highway    | name   | oneway |
            | abc   | trunk      | first  |        |
            | cd    | trunk      | first  |        |
            | bhf   | trunk_link |        | yes    |
            | cfg   | primary    | second | yes    |
            | ec    | primary    | second |        |

        And the relations
            | type        | way:from | way:to | node:via | restriction   |
            | restriction | abc      | cfg    | c        | no_right_turn |

       When I route I should get
            | waypoints | route               | turns                    |
            | a,g       | first,second,second | depart,turn right,arrive |
            | a,1       | first,,             | depart,turn right,arrive |

    Scenario: Turn Instead of Ramp
        Given the node map
            """
                    e
            a b-----c-d
               `--h |
                   ||
                  1||
                   ||
                   `f
                    |
                    g
            """

        And the ways
            | nodes | highway    | name   | oneway | route |
            | abc   | trunk      | first  | yes    |       |
            | cd    | trunk      | first  | yes    |       |
            | bhf   | trunk_link |        | yes    | ferry |
            | cfg   | primary    | second | yes    |       |
            | ec    | primary    | second | yes    |       |

        And the relations
            | type        | way:from | way:to | node:via | restriction   |
            | restriction | abc      | cfg    | c        | no_right_turn |

       When I route I should get
            | waypoints | route                | turns                                  |
            | a,g       | first,,second,second | depart,turn right,turn straight,arrive |

    Scenario: Turning Sliproad onto a ferry
        Given the node map
            """
                    e
            a b-----c-d
               `--h |
                   ||
                  1||
                   ||
                   `f
                    |
                    g
                    |
                    i
            """

        And the ways
            | nodes | highway    | name   | oneway | route |
            | abc   | trunk      | first  |        |       |
            | cd    | trunk      | first  |        |       |
            | bhf   | trunk_link |        | yes    |       |
            | cf    | primary    | second | yes    |       |
            | fg    | primary    | second | yes    | ferry |
            | ec    | primary    | second | yes    |       |
            | gi    | primary    | second | yes    |       |

        And the relations
            | type        | way:from | way:to | node:via | restriction   |
            | restriction | abc      | cf     | c        | no_right_turn |

       When I route I should get
            | waypoints | route                       | turns                                                        |
            | a,i       | first,,second,second,second | depart,turn right,turn straight,notification straight,arrive |
            | a,1       | first,,                     | depart,turn right,arrive                                     |

    Scenario: Turn Instead of Ramp - Max-Speed
        Given the node map
            """
                    e
            a-b-----c-------------------------d
               `--h |
                   ||
                  1||
                   ||
                   `f
                    |
                    g
            """

        And the ways
            | nodes | highway    | name   | maxspeed | oneway |
            | abc   | trunk      | first  | 70       |        |
            | cd    | trunk      | first  | 2        |        |
            | bhf   | trunk_link |        | 2        | yes    |
            | cfg   | primary    | second | 50       | yes    |
            | ec    | primary    | second | 50       |        |

        And the relations
            | type        | way:from | way:to | node:via | restriction   |
            | restriction | abc      | cfg    | c        | no_right_turn |

       When I route I should get
            | waypoints | route               | turns                    |
            | a,g       | first,second,second | depart,turn right,arrive |
            | a,1       | first,,             | depart,turn right,arrive |


    Scenario: Turn Instead of Ramp
        Given the node map
            """
                    e
                    |
                    …
            a-b-----c--d
               `--h …
                   \|
                    |
                    |
                    |
                    |
                    |
                    |
                    f
                    |
                    |
                    g
            """

        And the ways
            | nodes | highway       | name   |
            | abcd  | motorway      | first  |
            | bhf   | motorway_link |        |
            | efg   | primary       | second |

       When I route I should get
            | waypoints | route                | turns                                      |
            | a,g       | first,,second,second | depart,off ramp right,turn straight,arrive |

    Scenario: Turn Instead of Ramp
        Given the node map
            """
                    e
                    |
                    …
            a-b-----c-d
               `--h …
                   \|
                    |
                    |
                    f
                    |
                    |
                    g
            """

        And the ways
            | nodes | highway       | name   |
            | abcd  | motorway      | first  |
            | bhf   | motorway_link |        |
            | efg   | primary       | second |

        When I route I should get
            | waypoints | route                | turns                                      |
            | a,g       | first,,second,second | depart,off ramp right,turn straight,arrive |

    Scenario: Inner city expressway with on road
        Given the node map
            """
            a b-------c-g
                 `--f |
                     \|
                      |
                      |
                      d
                      |
                      |
                      |
                      e
            """

        And the ways
            | nodes | highway      | name  | oneway |
            | abc   | primary      | road  |        |
            | cg    | primary      | road  |        |
            | bfd   | trunk_link   |       | yes    |
            | cde   | trunk        | trunk | yes    |

        And the relations
            | type        | way:from | way:to | node:via | restriction   |
            | restriction | abc      | cde    | c        | no_right_turn |

       When I route I should get
            | waypoints | route                | turns                    |
            | a,e       | road,trunk,trunk     | depart,turn right,arrive |


    Scenario: Slipway Round U-Turn
        Given the node map
            """
            a   f
            |   |
            b   e
            |\ /|
            | | |
            | g |
            |   |
            c   d
            """

        And the ways
            | nodes | highway      | name | oneway |
            | abc   | primary      | road | yes    |
            | bge   | primary_link |      | yes    |
            | def   | primary      | road | yes    |

       When I route I should get
            | waypoints | route          | turns                        |
            | a,f       | road,road,road | depart,continue uturn,arrive |

    Scenario: Slipway Steep U-Turn
        Given the node map
            """
            a   f
            |   |
            b   e
            |\g/|
            |   |
            |   |
            c   d
            """

        And the ways
            | nodes | highway      | name | oneway |
            | abc   | primary      | road | yes    |
            | bge   | primary_link |      | yes    |
            | def   | primary      | road | yes    |

       When I route I should get
            | waypoints | route          | turns                        |
            | a,f       | road,road,road | depart,continue uturn,arrive |

    Scenario: Schwarzwaldstrasse Autobahn
        Given the node map
            """
                  . i . . . . . h . . . . g
               .j      '.   . '
            a .           k
               '  b . r c . d . e . . . . f
                   .    .   .
                     .  .   .
                      . .   .
                      . .   .
                      l .   .
                      m .   .
                        n   q
                        .   .
                        .   .
                        .   .
                        o   p
            """

        And the nodes
            # the traffic light at `l` is not actually in the data, but necessary for the test to check everything
            # http://www.openstreetmap.org/#map=19/48.99211/8.40336
            | node | highway         |
            | r    | traffic_signals |
            | l    | traffic_signals |

        And the ways
            | nodes | highway        | name               | ref  | oneway |
            | abrcd | secondary      | Schwarzwaldstrasse | L561 | yes    |
            | hija  | secondary      | Schwarzwaldstrasse | L561 | yes    |
            | def   | secondary      | Ettlinger Strasse  |      | yes    |
            | gh    | secondary      | Ettlinger Strasse  |      | yes    |
            | blmn  | secondary_link |                    | L561 | yes    |
            | hkc   | secondary_link | Ettlinger Strasse  |      | yes    |
            | qdki  | secondary_link | Ettlinger Allee    |      | yes    |
            | cn    | secondary_link | Ettlinger Allee    |      | yes    |
            | no    | primary        | Ettlinger Allee    |      | yes    |
            | pq    | primary        | Ettlinger Allee    |      | yes    |
            | qe    | secondary_link | Ettlinger Allee    |      | yes    |

        When I route I should get
            | waypoints | route                                              | turns                    | ref        |
            | a,o       | Schwarzwaldstrasse,Ettlinger Allee,Ettlinger Allee | depart,turn right,arrive | L561,L561, |

    Scenario: Traffic Lights everywhere
        #http://map.project-osrm.org/?z=18&center=48.995336%2C8.383813&loc=48.995467%2C8.384548&loc=48.995115%2C8.382761&hl=en&alt=0
        Given the node map
            """
            a     k l     j
                      d b c i

                        e g

                        1
                          h

                          f
            """

        And the nodes
            | node | highway         |
            | b    | traffic_signals |
            | e    | traffic_signals |
            | g    | traffic_signals |

        And the ways
            | nodes  | highway        | name          | oneway |
            | aklbci | secondary      | Ebertstrasse  | yes    |
            | kdeh   | secondary_link |               | yes    |
            | jcghf  | primary        | Brauerstrasse | yes    |

        When I route I should get
            | waypoints | route                                    | turns                           |
            | a,i       | Ebertstrasse,Ebertstrasse                | depart,arrive                   |
            | a,l       | Ebertstrasse,Ebertstrasse                | depart,arrive                   |
            | a,f       | Ebertstrasse,Brauerstrasse,Brauerstrasse | depart,turn right,arrive        |
            | a,1       | Ebertstrasse,,                           | depart,turn slight right,arrive |

    #2839
    Scenario: Self-Loop
        Given the node map
            """
                                                    /-l-----k---\
                                                   /             `j--
                                                  m                  \
                                                 /                    i
                                                /                      \
                                                |                       \ 
                                                |                       h
                                                |                       |
                                                n                       |
                                                |                       |
                                                |                       g
                                                o                       |
                                               /                       /
                                              |                       f
                                           /- p                     /
                                          /                        e
            a ------- b --------------- c ----------------- d ---/
            """

     And the ways
            | nodes           | name    | oneway | highway     | lanes |
            | abc             | circled | no     | residential | 1     |
            | cdefghijklmnopc | circled | yes    | residential | 1     |

     When I route I should get
            | waypoints | bearings     | route           | turns         |
            | b,a       | 90,10 270,10 | circled,circled | depart,arrive |

    @todo
    #due to the current turn function, the left turn bcp is exactly the same cost as pcb, a right turn. The right turn should be way faster,though
    #for that reason we cannot distinguish between driving clockwise through the circle or counter-clockwise. Until this is improved, this case here
    #has to remain as todo (see #https://github.com/Project-OSRM/osrm-backend/pull/2849)
    Scenario: Self-Loop - Bidirectional
        Given the node map
            """
                                                    /-l-----k---\
                                                   /             `j--
                                                  m                  \
                                                 /                    i
                                                /                      \
                                                |                       \ 
                                                |                       h
                                                |                       |
                                                n                       |
                                                |                       |
                                                |                       g
                                                o                       |
                                               /                       /
                                              |                       f
                                           /- p                     /
                                          /                        e
            a ------- b --------------- c ----------------- d ---/
            """

     And the ways
            | nodes           | name    | oneway | highway     | lanes |
            | abc             | circled | no     | residential | 1     |
            | cdefghijklmnopc | circled | no     | residential | 1     |

     When I route I should get
            | waypoints | bearings     | route           | turns         |
            | b,a       | 90,10 270,10 | circled,circled | depart,arrive |

    #http://www.openstreetmap.org/#map=19/38.90597/-77.01276
    Scenario: Don't falsly classify as sliproads
        Given the node map
            """
                                                          j
            a-b ----------------------------------------- c ------------d
               \                                          |
                \                                         |
                 \                                        |
                  \                                       |
                   \                                      |
                    e                                     |
                     \                                    |
                      \                                   |
                       \                                  |
                        \                                 |
                         \                                |
                          \                               |
                           \                              |
                            \                             |
                             \                            |
                              \                           |
                               \                          |
                                \                         |
                                 \                        |
                                  \                       |
                                   \                      1
                                    `---------- f ------- g ----------\
                                                          |            \
                                                          i             h
            """

        And the ways
            | nodes | name       | highway   | oneway | maxspeed |
            | abcd  | new york   | primary   | yes    | 35       |
            | befgh | m street   | secondary | yes    | 35       |
            | igcj  | 1st street | tertiary  | no     | 20       |

        And the nodes
            | node | highway         |
            | c    | traffic_signals |
            | g    | traffic_signals |

        When I route I should get
            | waypoints | route                                   | turns                              | #                                    |
            | a,d       | new york,new york                       | depart,arrive                      | this is the sinatra route            |
            | a,j       | new york,1st street,1st street          | depart,turn left,arrive            |                                      |
            | a,1       | new york,m street,1st street,1st street | depart,turn right,turn left,arrive | this can false be seen as a sliproad |

    # Merging into degree two loop on dedicated turn detection / 2927
    Scenario: Turn Instead of Ramp
        Given the node map
            """
                     /--------------------f
                    g-----------h--\      |
                                    d-----e
            i       c-----------j--/
            |       |
            |       |
            |       |
            |       |
            |       |
             \     / 
              \   /
               \ /
                b
                |
                a
            """

        And the ways
            | nodes | highway | name | oneway |
            | abi   | primary | road | yes    |
            | bcjd  | primary | loop | yes    |
            | dhgf  | primary | loop | yes    |
            | fed   | primary | loop | yes    |

        And the nodes
            | node | highway         |
            | g    | traffic_signals |
            | c    | traffic_signals |

        # We don't actually care about routes here, this is all about endless loops in turn discovery
        When I route I should get
            | waypoints | route          | turns                          |
            | a,i       | road,road,road | depart,fork slight left,arrive |


    # The following tests are current false positives / false negatives #3199

    @sliproads
    # http://www.openstreetmap.org/#map=19/52.59847/13.14815
    Scenario: Sliproad Detection
        Given the node map
            """
            a                            . . .
              .                         .
                b  . . . . . .  c . . . d
                  `             .       .
                    e           .       .
                       `        .       .
                          f     .       .
                             `  .       .
                                g       i
                                   ` h .
            """

        And the ways
            | nodes  | highway     | name              |
            | abefgh | residential | Nachtigallensteig |
            | bcd    | residential | Kiebitzsteig      |
            | cg     | residential | Haenflingsteig    |
            | hid    | residential | Waldkauzsteig     |

       When I route I should get
            | waypoints | route                                        | turns                   |
            | a,d       | Nachtigallensteig,Kiebitzsteig,Kiebitzsteig  | depart,turn left,arrive |
            | a,h       | Nachtigallensteig,Nachtigallensteig          | depart,arrive           |


    @sliproads
    Scenario: Not a obvious Sliproad
        Given the node map
            """
                    d
                    .
          s . a . . b . . c
                `   .
                  ` e
                    .`
                    .  `
                    f    g
            """

        And the ways
            | nodes | highway | name  | oneway |
            | sabc  | primary | sabc  |        |
            | dbef  | primary | dbef  | yes    |
            | aeg   | primary | aeg   | yes    |

       When I route I should get
            | waypoints | route              | turns                               |
            | s,f       | sabc,aeg,dbef,dbef | depart,turn right,turn right,arrive |

    @sliproads
    Scenario: Through Street, not a Sliproad although obvious
        Given the node map
            """
                    d
                    .
          s . a . . b . . c
                `   .
                  ` e
                  .  `
                 .     `
                f        g
            """

        And the ways
            | nodes | highway | name  | oneway |
            | sabc  | primary | sabc  |        |
            | dbef  | primary | dbef  | yes    |
            | aeg   | primary | aeg   | yes    |

       When I route I should get
            | waypoints | route              | turns                               |
            | s,f       | sabc,aeg,dbef,dbef | depart,turn right,turn right,arrive |

    @sliproads
    Scenario: Sliproad target turn is restricted
        Given the node map
            """
                        d
                        .
          s . a . . . . b . . c
                `       .
                   `    .
                     `  .
                      ` .
                       `.
                        e
                        .`
                        f `
                        .  ` g
            """

        And the ways
            | nodes | highway | name | oneway |
            | sa    | primary | sabc |        |
            | abc   | primary | sabc |        |
            | dbe   | primary | dbef | yes    |
            | ef    | primary | dbef |        |
            | ae    | primary | aeg  | yes    |
            | eg    | primary | aeg  |        |
            # the reason we have to split ways at e is that otherwise we can't handle restrictions via e

        And the relations
            | type        | way:from | way:to | node:via | restriction   |
            | restriction | ae       | ef     | e        | no_right_turn |

       When I route I should get
            | waypoints | route          | turns                    |
            | s,f       | sabc,dbef,dbef | depart,turn right,arrive |
            | s,g       | sabc,aeg,aeg   | depart,turn right,arrive |

    @sliproads
    Scenario: Not a Sliproad, road not continuing straight
        Given the node map
            """
                    d
                    .
          s . a . . b . . c
                `   .
                  ` e . . g
            """

        And the ways
            | nodes | highway | name  | oneway |
            | sabc  | primary | sabc  |        |
            | dbe   | primary | dbe   | yes    |
            | aeg   | primary | aeg   | yes    |

       When I route I should get
            | waypoints | route        | turns                    |
            | s,c       | sabc,sabc    | depart,arrive            |
            | s,g       | sabc,aeg,aeg | depart,turn right,arrive |

    @sliproads
    Scenario: Intersection too far away with Traffic Light shortly after initial split
        Given the node map
            """
                                                                                                                                                                      d
                                                                                                                                                                      .
          s . a . . . . . . . . . . . . . t . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . b . . c
               ` . . . . . . . . . .                                                                                                                                  .                   .
                                      `  . . . . . . . . . .                                                                                                          .                                .
                                                              `  . . . . . . . . . .                                                                                  .                                        .
                                                                                      `  . . . . . . . . . .                                                          .                                                     .
                                                                                                              `  . . . . . . . . . .                                  .                                                                 .
                                                                                                                                      `  . . . . . . . . . .          .                                                                              .
                                                                                                                                                              `  .    .                     .
                                                                                                                                                                   `  e
                                                                                                                                                                      .
                                                                                                                                                                      f
                                                                                                                                                                      .
            """

        And the nodes
            | node | highway         |
            | t    | traffic_signals |

        And the ways
            | nodes | highway | name | oneway |
            | satbc | primary | sabc |        |
            | dbef  | primary | dbef | yes    |
            | ae    | primary | ae   | yes    |

       When I route I should get
            | waypoints | route             | turns                                      |
            | s,f       | sabc,ae,dbef,dbef | depart,turn slight right,turn right,arrive |

    @sliproads
    Scenario: Traffic Signal on Sliproad
        Given the node map
            """
                          d
                          .
          s . a . . . . . b . . c
               `          .
                  `       .
                     `    .
                       t  .
                        ` .
                          e
                          .
                          .
                          f
            """

        And the nodes
            | node | highway         |
            | t    | traffic_signals |

        And the ways
            | nodes | highway | name  | oneway |
            | sabc  | primary | sabc  |        |
            | dbe   | primary | dbe   | yes    |
            | ef    | primary | ef    |        |
            | ate   | primary | ate   | yes    |

       When I route I should get
            | waypoints | route      | turns                    |
            | s,f       | sabc,ef,ef | depart,turn right,arrive |

    @sliproads
    Scenario: Sliproad tagged as link
        Given the node map
            """
                          d
                          .
          s . a . . . . . b . . c
               `          .
                  `       .
                     `    .
                       `  .
                        ` .
                          e
                          .
                          .
                          f
            """

        And the ways
            | nodes | highway       | name  | oneway |
            | sabc  | motorway      | sabc  |        |
            | dbef  | motorway      | dbef  | yes    |
            | ae    | motorway_link | ae    | yes    |

       When I route I should get
            | waypoints | route          | turns                    |
            | s,f       | sabc,dbef,dbef | depart,turn right,arrive |

    @sliproads
    Scenario: Sliproad with same-ish names
        Given the node map
            """
                    d
                    .
          s . a . . b . . c
               `    .
                 .  e
                   ..
                    .
                    f
                    .
                    t
            """

        And the ways
            | nodes | highway | name     | ref   | oneway |
            | sabc  | primary | main     |       |        |
            | dbe   | primary | crossing | r0    | yes    |
            | eft   | primary | crossing | r0;r1 | yes    |
            | af    | primary | sliproad |       | yes    |

       When I route I should get
            | waypoints | route                  | turns                    |
            | s,t       | main,crossing,crossing | depart,turn right,arrive |

    @sliproads
    Scenario: Not a Sliproad, name mismatch
        Given the node map
            """
                    d
                    .
          s . a . . b . . c
               `    .
                 .  e
                  . .
                   ..
                    .
                    f
                    .
                    t
            """

        And the ways
            | nodes | highway | name     | oneway |
            | sabc  | primary | main     |        |
            | dbe   | primary | top      | yes    |
            | ef    | primary | bottom   | yes    |
            | ft    | primary | away     | yes    |
            | af    | primary | sliproad | yes    |

       When I route I should get
            | waypoints | route          | turns                    |
            | s,t       | main,away,away | depart,turn right,arrive |

    @sliproads
    Scenario: Not a Sliproad, low road priority
        Given the node map
            """
                    d
                    .
          s . a . . b . . c
               `    .
                 .  e
                  . .
                   ..
                    .
                    f
                    .
                    t
            """

        And the ways
        # maxspeed otherwise service road will never be routed over and we won't see instructions
            | nodes | highway | name     | maxspeed | oneway |
            | sabc  | primary | main     | 30 km/h  |        |
            | dbe   | primary | crossing | 30 km/h  | yes    |
            | eft   | primary | crossing | 30 km/h  | yes    |
            | ft    | primary | away     | 30 km/h  | yes    |
            | af    | service | sliproad | 30 km/h  | yes    |

       When I route I should get
            | waypoints | route          | turns                    |
            | s,t       | main,away,away | depart,turn right,arrive |

    @sliproads
    Scenario: Not a Sliproad, more than three roads at target intersection
        Given the node map
            """
                    d
                    .
          s . a . . b . . c
               `    .
                 .  e
                  . .
                   ..
                    .   h
                    f .
                    .   g
                    t
            """

        And the ways
            | nodes | highway | name     | oneway |
            | sabc  | primary | main     |        |
            | dbe   | primary | top      | yes    |
            | eft   | primary | bottom   | yes    |
            | fh    | primary | another  |        |
            | fg    | primary | another  |        |
            | af    | primary | sliproad | yes    |

       When I route I should get
            | waypoints | route                         | turns                              |
            | s,g       | main,sliproad,another,another | depart,turn right,turn left,arrive |

    @sliproads:
    Scenario: Throughabout-Sliproad
        Given the node map
            """
                             t
                             |
                         - - e - -
                       /           \
                     |               |
                     |               |
            z - s -  a - - - - - - - b - - -x
                   ' c               y
                     |               |
                       \           /
                         - -d - -
            """

        And the ways
            | nodes | name    | highway    | oneway | junction   | #                    |
            | zs    | through | trunk      | yes    |            |                      |
            | sa    | through | trunk      | yes    |            |                      |
            | ab    | through | trunk      | yes    |            |                      |
            | bx    | through | trunk      | yes    |            |                      |
            | ac    | round   | primary    | yes    | roundabout |                      |
            | cdy   | round   | primary    | yes    | roundabout |                      |
            | yb    | round   | primary    | yes    | roundabout |                      |
            | be    | round   | primary    | yes    | roundabout |                      |
            | ea    | round   | primary    | yes    | roundabout |                      |
            | et    | out     | primary    | yes    |            | the extraterrestrial |
            | sc    |         | trunk_link | yes    |            |                      |
            | yx    | right   | trunk_link | yes    |            |                      |

        And the relations
            | type        | way:from | way:to | node:via | restriction   |
            | restriction | sa       | ab     | a        | only_straight |
            | restriction | ab       | bx     | b        | only_straight |
            | restriction | yb       | be     | b        | only_straight |

        When I route I should get
            | waypoints | route            | turns                                            | locations |
            | z,t       | through,,out,out | depart,off ramp slight right,round-exit-3,arrive | z,s,c,t   |

    Scenario: Sliproad before a roundabout
        Given the node map
            """
                      e
            a - b - - c - d
                    'f|l'
                      m
                      g
                      |
                     .h-_
                k - i    |
                     '.j.'

            """

        And the ways
            | nodes | junction   | oneway | highway   | name |
            | ab    |            | yes    | primary   | road |
            | bc    |            | yes    | primary   | road |
            | cd    |            | yes    | primary   | road |
            | ec    |            | yes    | secondary |      |
            | cm    |            | yes    | secondary |      |
            | mg    |            | yes    | primary   |      |
            | gh    |            | no     | primary   |      |
            | hijh  | roundabout | yes    | primary   |      |
            | ik    |            | yes    | primary   |      |
            | bfm   |            | yes    | primary   |      |
            | gld   |            | yes    | primary   |      |

		And the relations
            | type        | way:from | way:to | node:via | restriction   |
            | restriction | bc       | cd     | c        | only_straight |

        When I route I should get
            | waypoints | route     | turns                                                     | locations |
            | a,k       | road,,,   | depart,continue right,roundabout turn right exit-1,arrive | a,b,h,k   |
