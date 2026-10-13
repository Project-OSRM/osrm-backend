@routing  @guidance
Feature: Basic Roundabout

    Background:
        Given the profile "bicycle"
        Given a grid size of 10 meters

    # https://www.openstreetmap.org/way/223225602
    Scenario: Enter and Exit with changing mode
        Given the node map
            """
                a
                b
            h g   c d
                e
                f
            """

       And the ways
            | nodes | junction   | highway     |
            | ab    |            | residential |
            | cd    |            | residential |
            | ef    |            | footway     |
            | gh    |            | footway     |
            | bgecb | roundabout | residential |

       When I route I should get
            | waypoints | route       | turns                                                               | locations |
            | a,d       | ab,cd,cd    | depart,roundabout turn left exit-1,arrive                           | a,?,d     |
            | a,f       | ab,ef,ef,ef | depart,roundabout turn straight exit-1,exit roundabout right,arrive | a,?,e,f   |
            | a,h       | ab,gh,gh,gh | depart,roundabout turn right exit-1,exit roundabout right,arrive    | a,?,g,h   |
            | d,f       | cd,ef,ef,ef | depart,roundabout turn left exit-2,exit roundabout right,arrive     | d,?,e,f   |
            | d,h       | cd,gh,gh,gh | depart,roundabout turn straight exit-2,exit roundabout right,arrive | d,?,g,h   |
            | d,a       | cd,ab,ab    | depart,roundabout turn right exit-1,arrive                          | d,?,a     |
            | f,h       | ef,gh,gh,gh | depart,roundabout turn left exit-3,exit roundabout right,arrive     | f,?,g,h   |
            | f,a       | ef,ab,ab    | depart,roundabout turn straight exit-2,arrive                       | f,?,a     |
            | f,d       | ef,cd,cd    | depart,roundabout turn right exit-1,arrive                          | f,?,d     |
            | h,a       | gh,ab,ab    | depart,roundabout turn left exit-2,arrive                           | h,?,a     |
            | h,d       | gh,cd,cd    | depart,roundabout turn straight exit-1,arrive                       | h,?,d     |
            | h,f       | gh,ef,ef,ef | depart,roundabout turn right exit-1,exit roundabout right,arrive    | h,?,e,f   |
