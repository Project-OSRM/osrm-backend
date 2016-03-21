@routing @bicycle @mode
Feature: Bike - Mode flag

# bicycle modes:
# 3 bike
# 4 pushing
# 5 ferry
# 4 train

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
    	 | from | to | route    | turns                       | modes |
    	 | a    | d  | ab,bc,cd | head,right,left,destination | 3,5,3 |
    	 | d    | a  | cd,bc,ab | head,right,left,destination | 3,5,3 |
    	 | c    | a  | bc,ab    | head,left,destination       | 5,3   |
    	 | d    | b  | cd,bc    | head,right,destination      | 3,5   |
    	 | a    | c  | ab,bc    | head,right,destination      | 3,5   |
    	 | b    | d  | bc,cd    | head,left,destination       | 5,3   |

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
     	 | from | to | route    | turns                       | modes |
     	 | a    | d  | ab,bc,cd | head,right,left,destination | 3,6,3 |
     	 | d    | a  | cd,bc,ab | head,right,left,destination | 3,6,3 |
     	 | c    | a  | bc,ab    | head,left,destination       | 6,3   |
     	 | d    | b  | cd,bc    | head,right,destination      | 3,6   |
     	 | a    | c  | ab,bc    | head,right,destination      | 3,6   |
     	 | b    | d  | bc,cd    | head,left,destination       | 6,3   |

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
     	 | from | to | route    | turns                                      | modes |
     	 | a    | d  | ab,bc,cd | head,straight,straight,destination         | 3,3,3 |
     	 | d    | a  | cd,bc,ab | head,right,left,destination                | 3,4,3 |
     	 | c    | a  | bc,ab    | head,left,destination                      | 4,3   |
     	 | d    | b  | cd,bc    | head,right,destination                     | 3,4   |
     	 | a    | c  | ab,bc    | head,straight,destination                  | 3,3   |
     	 | b    | d  | bc,cd    | head,straight,destination                  | 3,3   |

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
     	 | from | to | route    | turns                       | modes |
     	 | a    | d  | ab,bc,cd | head,right,left,destination | 3,4,3 |
     	 | d    | a  | cd,bc,ab | head,right,left,destination | 3,4,3 |
     	 | c    | a  | bc,ab    | head,left,destination       | 4,3   |
     	 | d    | b  | cd,bc    | head,right,destination      | 3,4   |
     	 | a    | c  | ab,bc    | head,right,destination      | 3,4   |
     	 | b    | d  | bc,cd    | head,left,destination       | 4,3   |

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
     	 | from | to | route     | modes |
     	 | a    | f  | ab,bcd,df | 3,4,3 |
     	 | f    | a  | df,bcd,ab | 3,4,3 |
     	 | d    | a  | bcd,ab    | 4,3   |
     	 | f    | b  | df,bcd    | 3,4   |
     	 | a    | d  | ab,bcd    | 3,4   |
     	 | b    | f  | bcd,df    | 4,3   |

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
    	 | from | to | route    | turns                       | modes |
    	 | a    | d  | ab,bc,cd | head,right,left,destination | 3,4,3 |
    	 | d    | a  | cd,bc,ab | head,right,left,destination | 3,4,3 |
    	 | c    | a  | bc,ab    | head,left,destination       | 4,3   |
    	 | d    | b  | cd,bc    | head,right,destination      | 3,4   |
    	 | a    | c  | ab,bc    | head,right,destination      | 3,4   |
    	 | b    | d  | bc,cd    | head,left,destination       | 4,3   |

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
    	 | from | to | route    | turns                       | modes |
    	 | a    | d  | ab,bc,cd | head,right,left,destination | 3,4,3 |
    	 | d    | a  | cd,bc,ab | head,right,left,destination | 3,4,3 |
    	 | c    | a  | bc,ab    | head,left,destination       | 4,3   |
    	 | d    | b  | cd,bc    | head,right,destination      | 3,4   |
    	 | a    | c  | ab,bc    | head,right,destination      | 3,4   |
       | b    | d  | bc,cd    | head,left,destination       | 4,3   |

    Scenario: Bicycle - Modes when starting on forward oneway
        Given the node map
         | a | b |

        And the ways
         | nodes | oneway |
         | ab    | yes    |

        When I route I should get
         | from | to | route | modes |
         | a    | b  | ab    | 3     |
         | b    | a  | ab    | 4     |

    Scenario: Bicycle - Modes when starting on reverse oneway
        Given the node map
         | a | b |

        And the ways
         | nodes | oneway |
         | ab    | -1     |

        When I route I should get
         | from | to | route | modes |
         | a    | b  | ab    | 4     |
         | b    | a  | ab    | 3     |
