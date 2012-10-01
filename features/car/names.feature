@routing @car @names
Feature: Car - Street names in instructions

	Background:
		Given the speedprofile "car"
	
	Scenario: Car - A named street
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
	
	Scenario: Car - Use way type to describe unnamed ways
		Given the node map
		 | a | b | c |

		And the ways
		 | nodes | highway     | name |
		 | ab    | tertiary    |      |
		 | bc    | residential |      |

		When I route I should get
		 | from | to | route                |
		 | a    | c  | tertiary,residential |

	Scenario: Car - Don't create instructions for every node of unnamed ways
		Given the node map
		 | a | b | c | d |

		And the ways
		 | nodes | highway | name |
		 | abcd  | primary |      |

		When I route I should get
		 | from | to | route   |
		 | a    | d  | primary |
