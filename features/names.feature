@routing @names
Feature: Street names in instructions
	
	Scenario: A named street
		Given the node map
		 | a | b |
		 |   | c |
	
		And the ways
		 | nodes | name   |
		 | ab    | My Way |
		 | bc    | Your Way |
    
		When I route I should get
		 | from | to | route           |
		 | a    | c  | My Way,Your Way |
		
	Scenario: Use way type to describe unnamed ways
		Given the node map
		 | a | b | c |

		And the ways
		 | nodes | highway  | name |
		 | ab    | cycleway |      |
		 | bc    | track    |      |

		When I route I should get
		 | from | to | route         |
		 | a    | c  | cycleway,trac |
	
	Scenario: Don't create instructions for every node of unnamed ways
		Given the node map
		 | a | b | c | d |

		And the ways
		 | nodes | highway  | name |
		 | abcd  | cycleway |      |

		When I route I should get
		 | from | to | route    |
		 | a    | d  | cycleway |