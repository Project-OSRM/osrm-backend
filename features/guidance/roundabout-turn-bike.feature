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
           | waypoints | route    	 | turns                                         							|
		   | a,d       | ab,cd,cd,cd | depart,roundabout turn left exit-3,exit roundabout turn right,arrive 	|
		   | a,f       | ab,ef,ef,ef | depart,roundabout turn straight exit-2,exit roundabout turn right,arrive |
		   | a,h       | ab,gh,gh,gh | depart,roundabout turn right exit-1,exit roundabout turn right,arrive 	|
		   | d,f       | cd,ef,ef,ef | depart,roundabout turn left exit-3,exit roundabout turn right,arrive 	|
		   | d,h       | cd,gh,gh,gh | depart,roundabout turn straight exit-2,exit roundabout turn right,arrive |
		   | d,a       | cd,ab,ab,ab | depart,roundabout turn right exit-1,exit roundabout turn right,arrive 	|
		   | f,h       | ef,gh,gh,gh | depart,roundabout turn left exit-3,exit roundabout turn right,arrive 	|
		   | f,a       | ef,ab,ab,ab | depart,roundabout turn straight exit-2,exit roundabout turn right,arrive |
		   | f,d       | ef,cd,cd,cd | depart,roundabout turn right exit-1,exit roundabout turn right,arrive 	|
		   | h,a       | gh,ab,ab,ab | depart,roundabout turn left exit-3,exit roundabout turn right,arrive 	|
		   | h,d       | gh,cd,cd,cd | depart,roundabout turn straight exit-2,exit roundabout turn right,arrive |
		   | h,f       | gh,ef,ef,ef | depart,roundabout turn right exit-1,exit roundabout turn right,arrive 	|

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
           | waypoints | route       | turns                                                              		|
		   | a,d       | ab,cd,cd,cd | depart,roundabout turn left exit-1,exit roundabout turn right,arrive 	|
		   | a,f       | ab,ef,ef,ef | depart,roundabout turn straight exit-1,exit roundabout turn right,arrive |
		   | a,h       | ab,gh,gh,gh | depart,roundabout turn right exit-1,exit roundabout turn right,arrive 	|
		   | d,f       | cd,ef,ef,ef | depart,roundabout turn left exit-2,exit roundabout turn right,arrive 	|
		   | d,h       | cd,gh,gh,gh | depart,roundabout turn straight exit-2,exit roundabout turn right,arrive |
		   | d,a       | cd,ab,ab,ab | depart,roundabout turn right exit-1,exit roundabout turn right,arrive 	|
		   | f,h       | ef,gh,gh,gh | depart,roundabout turn left exit-3,exit roundabout turn right,arrive 	|
		   | f,a       | ef,ab,ab,ab | depart,roundabout turn straight exit-2,exit roundabout turn right,arrive |
		   | f,d       | ef,cd,cd,cd | depart,roundabout turn right exit-1,exit roundabout turn right,arrive 	|
		   | h,a       | gh,ab,ab,ab | depart,roundabout turn left exit-2,exit roundabout turn right,arrive 	|
		   | h,d       | gh,cd,cd,cd | depart,roundabout turn straight exit-1,exit roundabout turn right,arrive |
		   | h,f       | gh,ef,ef,ef  | depart,roundabout turn right exit-1,exit roundabout turn right,arrive 	|
