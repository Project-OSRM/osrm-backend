@routing  @guidance
Feature: Divided road entry

    Background:
        Given the profile "car"
        Given a grid size of 5 meters

    Scenario: Join on a divided road named after the main road
        Given the node map
            """
            a-------b-----c
                    |
            d-------e-----f
                    |
                    |
                    g
            """

        And the ways
            | nodes  | name    | highway     | oneway |
            | abc    | main st | residential | -1     |
            | def    | main st | residential | yes    |
            | be     | main st | residential |        |
            | eg     | side st | residential |        |

       When I route I should get
            | waypoints | route                  | turns                   |
            | g,a       | side st,main st,main st| depart,end of road left,arrive |


    # Similar to previous one, but the joining way is tagged with the side-street name
    Scenario: Join on a divided road, named after the side street
        Given the node map
            """
            a-------b-----c
                    |
            d-------e-----f
                    |
                    |
                    g
            """

        And the ways
            | nodes  | name    | highway     | oneway |
            | abc    | main st | residential | -1     |
            | def    | main st | residential | yes    |
            | beg    | side st | residential |        |

       When I route I should get
            | waypoints | route    | turns                           |
            | g,a       | side st,main st,main st| depart,end of road left,arrive |


    # Center join named after crossroad
    Scenario: Crossing a divided road, named after side-street
        Given the node map
            """
                    h
                    |
            a-------b-----c
                    |
            d-------e-----f
                    |
                    |
                    g
            """

        And the ways
            | nodes  | name    | highway     | oneway |
            | abc    | main st | residential | -1     |
            | def    | main st | residential | yes    |
            | hbeg   | side st | residential |        |

       When I route I should get
            | waypoints | route    | turns                           |
            | g,a       | side st,main st,main st| depart,turn left,arrive |

    # Join named after divided road
    Scenario: Crossing a divided road, named after main street
        Given the node map
            """
                    h
                    |
            a-------b-----c
                    |
            d-------e-----f
                    |
                    |
                    g
            """

        And the ways
            | nodes  | name    | highway     | oneway |
            | abc    | main st | residential | -1     |
            | def    | main st | residential | yes    |
            | be     | main st | residential |        |
            | hb     | side st | residential |        |
            | eg     | side st | residential |        |

       When I route I should get
            | waypoints | route    | turns                           |
            | g,a       | side st,main st,main st| depart,turn left,arrive |
