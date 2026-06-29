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
            | waypoints | route       | turns                               | locations |
            | b,d       | bcegb,cd,cd | depart,exit roundabout right,arrive | b,c,d     |
            | b,f       | bcegb,ef,ef | depart,exit roundabout right,arrive | b,e,f     |
            | b,h       | bcegb,gh,gh | depart,exit roundabout right,arrive | b,g,h     |
            | c,f       | bcegb,ef,ef | depart,exit roundabout right,arrive | c,e,f     |
            | c,h       | bcegb,gh,gh | depart,exit roundabout right,arrive | c,g,h     |
            | c,a       | bcegb,ab,ab | depart,exit roundabout right,arrive | c,b,a     |
            | e,h       | bcegb,gh,gh | depart,exit roundabout right,arrive | e,g,h     |
            | e,a       | bcegb,ab,ab | depart,exit roundabout right,arrive | e,b,a     |
            | e,d       | bcegb,cd,cd | depart,exit roundabout right,arrive | e,c,d     |
            | g,a       | bcegb,ab,ab | depart,exit roundabout right,arrive | g,b,a     |
            | g,d       | bcegb,cd,cd | depart,exit roundabout right,arrive | g,c,d     |
            | g,f       | bcegb,ef,ef | depart,exit roundabout right,arrive | g,e,f     |
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
            | waypoints | route           | turns                                                    | locations |
            | a,c       | abc,abc,abc     | depart,exit roundabout right,arrive                      | a,a,c     |
            | a,l       | abc,jkl,jkl,jkl | depart,roundabout-exit-2,exit roundabout straight,arrive | a,?,j,l   |
            | a,i       | abc,ghi,ghi,ghi | depart,roundabout-exit-3,exit roundabout straight,arrive | a,?,g,i   |
            | a,f       | abc,def,def,def | depart,roundabout-exit-4,exit roundabout straight,arrive | a,?,d,f   |
            | d,f       | def,def,def     | depart,exit roundabout right,arrive                      | d,d,f     |
            | d,c       | def,abc,abc,abc | depart,roundabout-exit-2,exit roundabout straight,arrive | d,?,a,c   |
            | d,l       | def,jkl,jkl,jkl | depart,roundabout-exit-3,exit roundabout straight,arrive | d,?,j,l   |
            | d,i       | def,ghi,ghi,ghi | depart,roundabout-exit-4,exit roundabout straight,arrive | d,?,g,i   |
            | g,i       | ghi,ghi,ghi     | depart,exit roundabout right,arrive                      | g,g,i     |
            | g,f       | ghi,def,def,def | depart,roundabout-exit-2,exit roundabout straight,arrive | g,?,d,f   |
            | g,c       | ghi,abc,abc,abc | depart,roundabout-exit-3,exit roundabout straight,arrive | g,?,a,c   |
            | g,l       | ghi,jkl,jkl,jkl | depart,roundabout-exit-4,exit roundabout straight,arrive | g,?,j,l   |
            | j,l       | jkl,jkl,jkl     | depart,exit roundabout right,arrive                      | j,j,l     |
            | j,i       | jkl,ghi,ghi,ghi | depart,roundabout-exit-2,exit roundabout straight,arrive | j,?,g,i   |
            | j,f       | jkl,def,def,def | depart,roundabout-exit-3,exit roundabout straight,arrive | j,?,d,f   |
            | j,c       | jkl,abc,abc,abc | depart,roundabout-exit-4,exit roundabout straight,arrive | j,?,a,c   |
