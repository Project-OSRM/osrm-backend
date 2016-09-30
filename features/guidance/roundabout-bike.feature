@routing  @guidance
Feature: Basic Roundabout

    Background:
        Given the profile "bicycle"
        Given a grid size of 10 meters

    Scenario: Only Enter
        Given the node map
            """
                a
                b
            d c   g h
                e
                f
            """

       And the ways
            | nodes  | junction   |
            | ab     |            |
            | cd     |            |
            | ef     |            |
            | gh     |            |
            | bcegb  | roundabout |

       When I route I should get
           | waypoints | route          | turns                                   |
           | a,c       | ab,bcegb,bcegb | depart,roundabout-exit-undefined,arrive |
           | a,e       | ab,bcegb,bcegb | depart,roundabout-exit-undefined,arrive |
           | a,g       | ab,bcegb,bcegb | depart,roundabout-exit-undefined,arrive |
           | d,e       | cd,bcegb,bcegb | depart,roundabout-exit-undefined,arrive |
           | d,g       | cd,bcegb,bcegb | depart,roundabout-exit-undefined,arrive |
           | d,b       | cd,bcegb,bcegb | depart,roundabout-exit-undefined,arrive |
           | f,g       | ef,bcegb,bcegb | depart,roundabout-exit-undefined,arrive |
           | f,b       | ef,bcegb,bcegb | depart,roundabout-exit-undefined,arrive |
           | f,c       | ef,bcegb,bcegb | depart,roundabout-exit-undefined,arrive |
           | h,b       | gh,bcegb,bcegb | depart,roundabout-exit-undefined,arrive |
           | h,c       | gh,bcegb,bcegb | depart,roundabout-exit-undefined,arrive |
           | h,e       | gh,bcegb,bcegb | depart,roundabout-exit-undefined,arrive |

    Scenario: Only Exit
        Given the node map
            """
                a
                b
            d c   g h
                e
                f
            """

       And the ways
            | nodes  | junction   |
            | ab     |            |
            | cd     |            |
            | ef     |            |
            | gh     |            |
            | bcegb  | roundabout |

       When I route I should get
           | waypoints | route       | turns                           |
           | b,d       | bcegb,cd,cd | depart,roundabout-exit-1,arrive |
           | b,f       | bcegb,ef,ef | depart,roundabout-exit-2,arrive |
           | b,h       | bcegb,gh,gh | depart,roundabout-exit-3,arrive |
           | c,f       | bcegb,ef,ef | depart,roundabout-exit-1,arrive |
           | c,h       | bcegb,gh,gh | depart,roundabout-exit-2,arrive |
           | c,a       | bcegb,ab,ab | depart,roundabout-exit-3,arrive |
           | e,h       | bcegb,gh,gh | depart,roundabout-exit-1,arrive |
           | e,a       | bcegb,ab,ab | depart,roundabout-exit-2,arrive |
           | e,d       | bcegb,cd,cd | depart,roundabout-exit-3,arrive |
           | g,a       | bcegb,ab,ab | depart,roundabout-exit-1,arrive |
           | g,d       | bcegb,cd,cd | depart,roundabout-exit-2,arrive |
           | g,f       | bcegb,ef,ef | depart,roundabout-exit-3,arrive |
      #phantom node snapping can result in a full round-trip here, therefore we cannot test b->a and the other direct exits

    Scenario: Drive Around
        Given the node map
            """
                a
                b
            d c   g h
                e
                f
            """

       And the ways
            | nodes  | junction   |
            | ab     |            |
            | cd     |            |
            | ef     |            |
            | gh     |            |
            | bcegb  | roundabout |

       When I route I should get
           | waypoints | route       | turns         |
           | b,c       | bcegb,bcegb | depart,arrive |
           | b,e       | bcegb,bcegb | depart,arrive |
           | b,g       | bcegb,bcegb | depart,arrive |
           | c,e       | bcegb,bcegb | depart,arrive |
           | c,g       | bcegb,bcegb | depart,arrive |
           | c,b       | bcegb,bcegb | depart,arrive |
           | e,g       | bcegb,bcegb | depart,arrive |
           | e,b       | bcegb,bcegb | depart,arrive |
           | e,c       | bcegb,bcegb | depart,arrive |
           | g,b       | bcegb,bcegb | depart,arrive |
           | g,c       | bcegb,bcegb | depart,arrive |
           | g,e       | bcegb,bcegb | depart,arrive |

     Scenario: Mixed Entry and Exit
        Given the node map
           """
             c   a
           j   b   f
             k   e
           l   h   d
             g   i
           """

        And the ways
           | nodes | junction   | oneway |
           | abc   |            | yes    |
           | def   |            | yes    |
           | ghi   |            | yes    |
           | jkl   |            | yes    |
           | bkheb | roundabout | yes    |

        When I route I should get
           | waypoints | route       | turns                           |
           | a,c       | abc,abc,abc | depart,roundabout-exit-1,arrive |
           | a,l       | abc,jkl,jkl | depart,roundabout-exit-2,arrive |
           | a,i       | abc,ghi,ghi | depart,roundabout-exit-3,arrive |
           | a,f       | abc,def,def | depart,roundabout-exit-4,arrive |
           | d,f       | def,def,def | depart,roundabout-exit-1,arrive |
           | d,c       | def,abc,abc | depart,roundabout-exit-2,arrive |
           | d,l       | def,jkl,jkl | depart,roundabout-exit-3,arrive |
           | d,i       | def,ghi,ghi | depart,roundabout-exit-4,arrive |
           | g,i       | ghi,ghi,ghi | depart,roundabout-exit-1,arrive |
           | g,f       | ghi,def,def | depart,roundabout-exit-2,arrive |
           | g,c       | ghi,abc,abc | depart,roundabout-exit-3,arrive |
           | g,l       | ghi,jkl,jkl | depart,roundabout-exit-4,arrive |
           | j,l       | jkl,jkl,jkl | depart,roundabout-exit-1,arrive |
           | j,i       | jkl,ghi,ghi | depart,roundabout-exit-2,arrive |
           | j,f       | jkl,def,def | depart,roundabout-exit-3,arrive |
           | j,c       | jkl,abc,abc | depart,roundabout-exit-4,arrive |
