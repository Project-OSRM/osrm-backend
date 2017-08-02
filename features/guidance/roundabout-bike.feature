@routing  @guidance
Feature: Basic Roundabout

    Background:
        Given the profile "bicycle"
        Given a grid size of 10 meters

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
           | waypoints | route       | turns                               |
           | b,d       | bcegb,cd,cd | depart,exit roundabout right,arrive |
           | b,f       | bcegb,ef,ef | depart,exit roundabout right,arrive |
           | b,h       | bcegb,gh,gh | depart,exit roundabout right,arrive |
           | c,f       | bcegb,ef,ef | depart,exit roundabout right,arrive |
           | c,h       | bcegb,gh,gh | depart,exit roundabout right,arrive |
           | c,a       | bcegb,ab,ab | depart,exit roundabout right,arrive |
           | e,h       | bcegb,gh,gh | depart,exit roundabout right,arrive |
           | e,a       | bcegb,ab,ab | depart,exit roundabout right,arrive |
           | e,d       | bcegb,cd,cd | depart,exit roundabout right,arrive |
           | g,a       | bcegb,ab,ab | depart,exit roundabout right,arrive |
           | g,d       | bcegb,cd,cd | depart,exit roundabout right,arrive |
           | g,f       | bcegb,ef,ef | depart,exit roundabout right,arrive |
      #phantom node snapping can result in a full round-trip here, therefore we cannot test b->a and the other direct exits

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
           | waypoints | route           | turns                                                    |
           | a,c       | abc,abc,abc     | depart,exit roundabout right,arrive                      |
           | a,l       | abc,jkl,jkl,jkl | depart,roundabout-exit-2,exit roundabout straight,arrive |
           | a,i       | abc,ghi,ghi,ghi | depart,roundabout-exit-3,exit roundabout straight,arrive |
           | a,f       | abc,def,def,def | depart,roundabout-exit-4,exit roundabout straight,arrive |
           | d,f       | def,def,def     | depart,exit roundabout right,arrive                      |
           | d,c       | def,abc,abc,abc | depart,roundabout-exit-2,exit roundabout straight,arrive |
           | d,l       | def,jkl,jkl,jkl | depart,roundabout-exit-3,exit roundabout straight,arrive |
           | d,i       | def,ghi,ghi,ghi | depart,roundabout-exit-4,exit roundabout straight,arrive |
           | g,i       | ghi,ghi,ghi     | depart,exit roundabout right,arrive                      |
           | g,f       | ghi,def,def,def | depart,roundabout-exit-2,exit roundabout straight,arrive |
           | g,c       | ghi,abc,abc,abc | depart,roundabout-exit-3,exit roundabout straight,arrive |
           | g,l       | ghi,jkl,jkl,jkl | depart,roundabout-exit-4,exit roundabout straight,arrive |
           | j,l       | jkl,jkl,jkl     | depart,exit roundabout right,arrive                      |
           | j,i       | jkl,ghi,ghi,ghi | depart,roundabout-exit-2,exit roundabout straight,arrive |
           | j,f       | jkl,def,def,def | depart,roundabout-exit-3,exit roundabout straight,arrive |
           | j,c       | jkl,abc,abc,abc | depart,roundabout-exit-4,exit roundabout straight,arrive |
