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
           | waypoints | route       | turns | locations |
            | a,d | ab,cd,cd,cd | depart,bgecb-exit-3,exit rotary right,arrive | a,?,c,d |
            | a,f | ab,ef,ef,ef | depart,bgecb-exit-2,exit rotary right,arrive | a,?,e,f |
            | a,h | ab,gh,gh,gh | depart,bgecb-exit-1,exit rotary right,arrive | a,?,g,h |
            | d,f | cd,ef,ef,ef | depart,bgecb-exit-3,exit rotary right,arrive | d,?,e,f |
            | d,h | cd,gh,gh,gh | depart,bgecb-exit-2,exit rotary right,arrive | d,?,g,h |
            | d,a | cd,ab,ab,ab | depart,bgecb-exit-1,exit rotary right,arrive | d,?,a,a |
            | f,h | ef,gh,gh,gh | depart,bgecb-exit-3,exit rotary right,arrive | f,?,g,h |
            | f,a | ef,ab,ab,ab | depart,bgecb-exit-2,exit rotary right,arrive | f,?,a,a |
            | f,d | ef,cd,cd,cd | depart,bgecb-exit-1,exit rotary right,arrive | f,?,c,d |
            | h,a | gh,ab,ab,ab | depart,bgecb-exit-3,exit rotary right,arrive | h,?,a,a |
            | h,d | gh,cd,cd,cd | depart,bgecb-exit-2,exit rotary right,arrive | h,?,c,d |
            | h,f | gh,ef,ef,ef | depart,bgecb-exit-1,exit rotary right,arrive | h,?,e,f |
