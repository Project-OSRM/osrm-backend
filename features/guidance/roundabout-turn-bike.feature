@routing  @guidance
Feature: Basic Roundabout

    Background:
        Given the profile "bicycle"
        Given a grid size of 10 meters

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
            | nodes  | junction   |
            | ab     |            |
            | cd     |            |
            | ef     |            |
            | gh     |            |
            | bgecb  | roundabout |

       When I route I should get
           | waypoints | route    | turns                                         |
           | a,d       | ab,cd,cd | depart,roundabout turn left exit-3,arrive     |
           | a,f       | ab,ef,ef | depart,roundabout turn straight exit-2,arrive |
           | a,h       | ab,gh,gh | depart,roundabout turn right exit-1,arrive    |
           | d,f       | cd,ef,ef | depart,roundabout turn left exit-3,arrive     |
           | d,h       | cd,gh,gh | depart,roundabout turn straight exit-2,arrive |
           | d,a       | cd,ab,ab | depart,roundabout turn right exit-1,arrive    |
           | f,h       | ef,gh,gh | depart,roundabout turn left exit-3,arrive     |
           | f,a       | ef,ab,ab | depart,roundabout turn straight exit-2,arrive |
           | f,d       | ef,cd,cd | depart,roundabout turn right exit-1,arrive    |
           | h,a       | gh,ab,ab | depart,roundabout turn left exit-3,arrive     |
           | h,d       | gh,cd,cd | depart,roundabout turn straight exit-2,arrive |
           | h,f       | gh,ef,ef | depart,roundabout turn right exit-1,arrive    |
