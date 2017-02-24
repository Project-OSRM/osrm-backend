@routing @bicycle @pushing
Feature: Bike - Accessability of different way types

    Background:
        Given the profile "bicycle"

    Scenario: Bike - Pushing bikes on pedestrian-only ways
        Then routability should be
            | highway    | oneway | forw         | backw        |
            | (nil)      |        |              |              |
            | cycleway   |        | cycling      | cycling      |
            | primary    |        | cycling      | cycling      |
            | pedestrian |        | pushing bike | pushing bike |
            | cycleway   |        | cycling      | cycling      |
            | primary    | yes    | cycling      | pushing bike |

    Scenario: Bike - Pushing bikes against normal oneways
        Then routability should be
            | highway    | oneway | forw         | backw        |
            | (nil)      |        |              |              |
            | primary    | yes    | cycling      | pushing bike |
            | pedestrian | yes    | pushing bike | pushing bike |

    Scenario: Bike - Pushing bikes against reverse oneways
        Then routability should be
            | highway    | oneway | forw         | backw        |
            | (nil)      |        |              |              |
            | primary    | -1     | pushing bike | cycling      |
            | pedestrian | -1     | pushing bike | pushing bike |

    @square
    Scenario: Bike - Push bikes on pedestrian areas
        Given the node map
            """
            x
            a b
            d c
            """

        And the ways
            | nodes | area | highway    |
            | xa    |      | primary    |
            | abcda | yes  | pedestrian |

        When I route I should get
            | from | to | route       |
            | a    | b  | abcda,abcda |
            | a    | d  | abcda,abcda |
            | b    | c  | abcda,abcda |
            | c    | b  | abcda,abcda |
            | c    | d  | abcda,abcda |
            | d    | c  | abcda,abcda |
            | d    | a  | abcda,abcda |
            | a    | d  | abcda,abcda |

    Scenario: Bike - Pushing bikes on ways with foot=yes
        Then routability should be
            | highway  | foot | forw         | backw        |
            | motorway |      |              |              |
            | motorway | yes  | pushing bike |              |
            | runway   |      |              |              |
            | runway   | yes  | pushing bike | pushing bike |

    @todo
    Scenario: Bike - Pushing bikes on ways with foot=yes in one direction
        Then routability should be
            | highway  | foot:forward | foot:backward | forw         | backw        |
            | motorway |              |               |              |              |
            | motorway | yes          |               | pushing bike |              |
            | motorway |              | yes           |              | pushing bike |

    @construction
    Scenario: Bike - Don't allow routing on ways still under construction
        Then routability should be
            | highway      | foot | bicycle | bothw |
            | primary      |      |         | x     |
            | construction |      |         |       |
            | construction | yes  |         |       |
            | construction |      | yes     |       |

    @roundabout
    Scenario: Bike - Don't push bikes against oneway flow on roundabouts
        Then routability should be
            | junction   | forw | backw |
            | roundabout | x    |       |

    Scenario: Bike - Instructions when pushing bike on oneways
        Given the node map
            """
            a b e
            f c d
            """

        And the ways
            | nodes | highway | oneway |
            | ab    | primary |        |
            | bc    | primary | yes    |
            | cd    | primary |        |
            | be    | primary |        |
            | cf    | primary |        |

        When I route I should get
            | from | to | route       | modes                                |
            | a    | d  | ab,bc,cd,cd | cycling,cycling,cycling,cycling      |
            | d    | a  | cd,bc,ab,ab | cycling,pushing bike,cycling,cycling |
            | c    | a  | bc,ab,ab    | pushing bike,cycling,cycling         |
            | d    | b  | cd,bc,bc    | cycling,pushing bike,pushing bike    |

    Scenario: Bike - Instructions when pushing bike on footway/pedestrian, etc.
        Given the node map
            """
            a b
              c d
            """

        And the ways
            | nodes | highway |
            | ab    | primary |
            | bc    | footway |
            | cd    | primary |

        When I route I should get
            | from | to | route       | modes                                |
            | a    | d  | ab,bc,cd,cd | cycling,pushing bike,cycling,cycling |
            | d    | a  | cd,bc,ab,ab | cycling,pushing bike,cycling,cycling |
            | c    | a  | bc,ab,ab    | pushing bike,cycling,cycling         |
            | d    | b  | cd,bc,bc    | cycling,pushing bike,pushing bike    |
