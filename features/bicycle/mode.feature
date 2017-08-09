@routing @bicycle @mode
Feature: Bike - Mode flag

	Background:
		Given the profile "bicycle"

    Scenario: Bike - Mode when using a ferry
    	Given the node map
    	 """
    	 a b
    	   c d
    	 """

    	And the ways
    	 | nodes | highway | route | duration |
    	 | ab    | primary |       |          |
    	 | bc    |         | ferry | 0:01     |
    	 | cd    | primary |       |          |

    	When I route I should get
    	 | from | to | route       | modes                         |
    	 | a    | d  | ab,bc,cd,cd | cycling,ferry,cycling,cycling |
    	 | c    | a  | bc,ab,ab    | ferry,cycling,cycling         |
    	 | d    | b  | cd,bc,bc    | cycling,ferry,ferry           |

     Scenario: Bike - Mode when using a train
     	Given the node map
     	 """
     	 a b
     	   c d
     	 """

     	And the ways
     	 | nodes | highway | railway | bicycle |
     	 | ab    | primary |         |         |
     	 | bc    |         | train   | yes     |
     	 | cd    | primary |         |         |

     	When I route I should get
     	 | from | to | route       | modes                         |
     	 | a    | d  | ab,bc,cd,cd | cycling,train,cycling,cycling |
     	 | c    | a  | bc,ab,ab    | train,cycling,cycling         |
     	 | d    | b  | cd,bc,bc    | cycling,train,train           |

     #representative test for all pushes (and mode changes). Where a bike is pushed is tested over in access.feature
     Scenario: Bike - Mode when pushing bike against oneways
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

    Scenario: Bicycle - Modes when starting on forward oneway
        Given the node map
         """
         a b
         """

        And the ways
         | nodes | oneway |
         | ab    | yes    |

        When I route I should get
         | from | to | route | modes                     |
         | a    | b  | ab,ab | cycling,cycling           |
         | b    | a  | ab,ab | pushing bike,pushing bike |

    Scenario: Bicycle - Modes when starting on reverse oneway
        Given the node map
         """
         a b
         """

        And the ways
         | nodes | oneway |
         | ab    | -1     |

        When I route I should get
         | from | to | route | modes                     |
         | a    | b  | ab,ab | pushing bike,pushing bike |
         | b    | a  | ab,ab | cycling,cycling           |
