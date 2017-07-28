@routing  @guidance
Feature: Circular

    # Circular tags are treated just as rotaries. We can rely on the rotary tests for their handling on special cases.
    # Here we only ensure that the `circular` tag is handled and assigned a rotary type

    Background:
        Given the profile "car"
        Given a grid size of 30 meters

    Scenario: Enter and Exit
        Given the node map
            """
                a
                b
            h g   c d
                e
                f
            """

       And the ways
            | nodes  | junction |
            | ab     |          |
            | cd     |          |
            | ef     |          |
            | gh     |          |
            | bgecb  | circular |

       When I route I should get
           | waypoints | route       | turns                                        |
           | a,d       | ab,cd,cd,cd | depart,bgecb-exit-3,exit rotary right,arrive |
           | a,f       | ab,ef,ef,ef | depart,bgecb-exit-2,exit rotary right,arrive |
           | a,h       | ab,gh,gh,gh | depart,bgecb-exit-1,exit rotary right,arrive |
           | d,f       | cd,ef,ef,ef | depart,bgecb-exit-3,exit rotary right,arrive |
           | d,h       | cd,gh,gh,gh | depart,bgecb-exit-2,exit rotary right,arrive |
           | d,a       | cd,ab,ab,ab | depart,bgecb-exit-1,exit rotary right,arrive |
           | f,h       | ef,gh,gh,gh | depart,bgecb-exit-3,exit rotary right,arrive |
           | f,a       | ef,ab,ab,ab | depart,bgecb-exit-2,exit rotary right,arrive |
           | f,d       | ef,cd,cd,cd | depart,bgecb-exit-1,exit rotary right,arrive |
           | h,a       | gh,ab,ab,ab | depart,bgecb-exit-3,exit rotary right,arrive |
           | h,d       | gh,cd,cd,cd | depart,bgecb-exit-2,exit rotary right,arrive |
           | h,f       | gh,ef,ef,ef | depart,bgecb-exit-1,exit rotary right,arrive |
