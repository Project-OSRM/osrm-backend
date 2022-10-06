@routing @bicycle @mode
Feature: Bicycle - Mode flag
    Background:
        Given the profile "bicycle"

    Scenario: Bicycle - We tag ferries with a class
        Given the node map
            """
            a b
              c d
            """

        And the ways
            | nodes | highway | route |
            | ab    | primary |       |
            | bc    |         | ferry |
            | cd    | primary |       |

        When I route I should get
            | from | to | route       | turns                                              | classes                  |
            | a    | d  | ab,bc,cd,cd | depart,notification right,notification left,arrive | [()],[(ferry)],[()],[()] |
            | d    | a  | cd,bc,ab,ab | depart,notification right,notification left,arrive | [()],[(ferry)],[()],[()] |
            | c    | a  | bc,ab,ab    | depart,notification left,arrive                    | [(ferry)],[()],[()]      |
            | d    | b  | cd,bc,bc    | depart,notification right,arrive                   | [()],[(ferry)],[()]      |
            | a    | c  | ab,bc,bc    | depart,notification right,arrive                   | [()],[(ferry)],[()]      |
            | b    | d  | bc,cd,cd    | depart,notification left,arrive                    | [(ferry)],[()],[()]      |

    Scenario: Bicycle - We tag tunnel with a class
        Background:
            Given a grid size of 200 meters

        Given the node map
            """
            a b
              c d
            """

        And the ways
            | nodes | tunnel               |
            | ab    | no                   |
            | bc    | yes                  |
            | cd    |                      |

        When I route I should get
            | from | to | route       | turns                                      | classes                   |
            | a    | d  | ab,bc,cd,cd | depart,new name right,new name left,arrive | [()],[(tunnel)],[()],[()] |

    Scenario: Bicycle - We tag classes without intersections
        Background:
            Given a grid size of 200 meters

        Given the node map
            """
            a b c d
            """

        And the ways
            | nodes | name | tunnel |
            | ab    | road |        |
            | bc    | road | yes    |
            | cd    | road |        |

        When I route I should get
          | from | to | route     | turns         | classes               |
          | a    | d  | road,road | depart,arrive | [(),(tunnel),()],[()] |

    Scenario: Bicycle - From roundabout on ferry
        Given the node map
            """
                     c
                  /     \
            a---b         d---f--h
                  \     /
                     e
                     |
                     g
            """

        And the ways
            | nodes | oneway | highway | junction   | route    |
            | ab    | yes    | service |            |          |
            | cb    | yes    | service | roundabout |          |
            | dc    | yes    | service | roundabout |          |
            | be    | yes    | service | roundabout |          |
            | ed    | yes    | service | roundabout |          |
            | eg    | yes    | service |            |          |
            | df    |        |         |            | ferry    |
            | fh    | yes    | service |            |          |

        When I route I should get
            | from | to | route          | turns                                                                              | classes                          |
            | a    | h  | ab,df,df,fh,fh | depart,roundabout-exit-2,exit roundabout slight right,notification straight,arrive | [()],[(),()],[(ferry)],[()],[()] |
