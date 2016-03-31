@routing @bicycle @mode
Feature: Bike - Mode flag

	Background:
		Given the profile "bicycle"

    Scenario: Bike - Mode when using a ferry
    	Given the node map
    	 | a | b |   |
    	 |   | c | d |

    	And the ways
    	 | nodes | highway | route | duration |
    	 | ab    | primary |       |          |
    	 | bc    |         | ferry | 0:01     |
    	 | cd    | primary |       |          |

    	When I route I should get
    	 | from | to | route       | turns                    | modes                         |
    	 | a    | d  | ab,bc,cd,cd | depart,right,left,arrive | cycling,ferry,cycling,cycling |
    	 | d    | a  | cd,bc,ab,ab | depart,right,left,arrive | cycling,ferry,cycling,cycling |
    	 | c    | a  | bc,ab,ab    | depart,left,arrive       | ferry,cycling,cycling         |
    	 | d    | b  | cd,bc,bc    | depart,right,arrive      | cycling,ferry,ferry           |
    	 | a    | c  | ab,bc,bc    | depart,right,arrive      | cycling,ferry,ferry           |
    	 | b    | d  | bc,cd,cd    | depart,left,arrive       | ferry,cycling,cycling         |

     Scenario: Bike - Mode when using a train
     	Given the node map
     	 | a | b |   |
     	 |   | c | d |

     	And the ways
     	 | nodes | highway | railway | bicycle |
     	 | ab    | primary |         |         |
     	 | bc    |         | train   | yes     |
     	 | cd    | primary |         |         |

     	When I route I should get
     	 | from | to | route       | turns                    | modes                         |
     	 | a    | d  | ab,bc,cd,cd | depart,right,left,arrive | cycling,train,cycling,cycling |
     	 | d    | a  | cd,bc,ab,ab | depart,right,left,arrive | cycling,train,cycling,cycling |
     	 | c    | a  | bc,ab,ab    | depart,left,arrive       | train,cycling,cycling         |
     	 | d    | b  | cd,bc,bc    | depart,right,arrive      | cycling,train,train           |
     	 | a    | c  | ab,bc,bc    | depart,right,arrive      | cycling,train,train           |
     	 | b    | d  | bc,cd,cd    | depart,left,arrive       | train,cycling,cycling         |

     Scenario: Bike - Mode when pushing bike against oneways
     	Given the node map
     	 | a | b |   |
     	 |   | c | d |

     	And the ways
     	 | nodes | highway | oneway |
     	 | ab    | primary |        |
     	 | bc    | primary | yes    |
     	 | cd    | primary |        |

     	When I route I should get
     	 | from | to | route       | turns                                   | modes                                |
     	 | a    | d  | ab,bc,cd,cd | depart,right,left,arrive                | cycling,cycling,cycling,cycling      |
     	 | d    | a  | cd,bc,ab,ab | depart,right,left,arrive                | cycling,pushing bike,cycling,cycling |
     	 | c    | a  | bc,ab,ab    | depart,left,arrive                      | pushing bike,cycling,cycling         |
     	 | d    | b  | cd,bc,bc    | depart,right,arrive                     | cycling,pushing bike,pushing bike    |
     	 | a    | c  | ab,bc,bc    | depart,right,arrive                     | cycling,cycling,cycling              |
     	 | b    | d  | bc,cd,cd    | depart,left,arrive                      | cycling,cycling,cycling              |

     Scenario: Bike - Mode when pushing on pedestrain streets
     	Given the node map
     	 | a | b |   |
     	 |   | c | d |

     	And the ways
     	 | nodes | highway    |
     	 | ab    | primary    |
     	 | bc    | pedestrian |
     	 | cd    | primary    |

     	When I route I should get
     	 | from | to | route       | turns                    | modes                                |
     	 | a    | d  | ab,bc,cd,cd | depart,right,left,arrive | cycling,pushing bike,cycling,cycling |
     	 | d    | a  | cd,bc,ab,ab | depart,right,left,arrive | cycling,pushing bike,cycling,cycling |
     	 | c    | a  | bc,ab,ab    | depart,left,arrive       | pushing bike,cycling,cycling         |
     	 | d    | b  | cd,bc,bc    | depart,right,arrive      | cycling,pushing bike,pushing bike    |
     	 | a    | c  | ab,bc,bc    | depart,right,arrive      | cycling,pushing bike,pushing bike    |
     	 | b    | d  | bc,cd,cd    | depart,left,arrive       | pushing bike,cycling,cycling         |

     Scenario: Bike - Mode when pushing on pedestrain areas
     	Given the node map
     	 | a | b |   |   |
     	 |   | c | d | f |

     	And the ways
     	 | nodes | highway    | area |
     	 | ab    | primary    |      |
     	 | bcd   | pedestrian | yes  |
     	 | df    | primary    |      |

     	When I route I should get
     	 | from | to | route        | modes                                |
     	 | a    | f  | ab,bcd,df,df | cycling,pushing bike,cycling,cycling |
     	 | f    | a  | df,bcd,ab,ab | cycling,pushing bike,cycling,cycling |
     	 | d    | a  | bcd,ab,ab    | pushing bike,cycling,cycling         |
     	 | f    | b  | df,bcd,bcd   | cycling,pushing bike,pushing bike    |
     	 | a    | d  | ab,bcd,bcd   | cycling,pushing bike,pushing bike    |
     	 | b    | f  | bcd,df,df    | pushing bike,cycling,cycling         |

     Scenario: Bike - Mode when pushing on steps
     	Given the node map
     	 | a | b |   |   |
     	 |   | c | d | f |

     	And the ways
    	 | nodes | highway |
    	 | ab    | primary |
    	 | bc    | steps   |
    	 | cd    | primary |

     	When I route I should get
    	 | from | to | route       | turns                    | modes                                |
    	 | a    | d  | ab,bc,cd,cd | depart,right,left,arrive | cycling,pushing bike,cycling,cycling |
    	 | d    | a  | cd,bc,ab,ab | depart,right,left,arrive | cycling,pushing bike,cycling,cycling |
    	 | c    | a  | bc,ab,ab    | depart,left,arrive       | pushing bike,cycling,cycling         |
    	 | d    | b  | cd,bc,bc    | depart,right,arrive      | cycling,pushing bike,pushing bike    |
    	 | a    | c  | ab,bc,bc    | depart,right,arrive      | cycling,pushing bike,pushing bike    |
    	 | b    | d  | bc,cd,cd    | depart,left,arrive       | pushing bike,cycling,cycling         |

     Scenario: Bike - Mode when bicycle=dismount
     	Given the node map
     	 | a | b |   |   |
     	 |   | c | d | f |

     	And the ways
    	 | nodes | highway | bicycle  |
    	 | ab    | primary |          |
    	 | bc    | primary | dismount |
    	 | cd    | primary |          |

     	When I route I should get
    	 | from | to | route       | turns                    | modes                                |
    	 | a    | d  | ab,bc,cd,cd | depart,right,left,arrive | cycling,pushing bike,cycling,cycling |
    	 | d    | a  | cd,bc,ab,ab | depart,right,left,arrive | cycling,pushing bike,cycling,cycling |
    	 | c    | a  | bc,ab,ab    | depart,left,arrive       | pushing bike,cycling,cycling         |
    	 | d    | b  | cd,bc,bc    | depart,right,arrive      | cycling,pushing bike,pushing bike    |
    	 | a    | c  | ab,bc,bc    | depart,right,arrive      | cycling,pushing bike,pushing bike    |
         | b    | d  | bc,cd,cd    | depart,left,arrive       | pushing bike,cycling,cycling         |

    Scenario: Bicycle - Modes when starting on forward oneway
        Given the node map
         | a | b |

        And the ways
         | nodes | oneway |
         | ab    | yes    |

        When I route I should get
         | from | to | route | modes                     |
         | a    | b  | ab,ab | cycling,cycling           |
         | b    | a  | ab,ab | pushing bike,pushing bike |

    Scenario: Bicycle - Modes when starting on reverse oneway
        Given the node map
         | a | b |

        And the ways
         | nodes | oneway |
         | ab    | -1     |

        When I route I should get
         | from | to | route | modes                     |
         | a    | b  | ab,ab | pushing bike,pushing bike |
         | b    | a  | ab,ab | cycling,cycling           |
