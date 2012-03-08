@routing @basic
Feature: Basic Routing
	
	@smallest
	Scenario: A single way with two nodes
		Given the node map
		 | a | b |
	
		And the ways
		 | nodes |
		 | ab    |
    
		When I route I should get
		 | from | to | route |
		 | a    | b  | ab    |
		 | b    | a  | ab    |
		
	Scenario: Routing in between two nodes of way
		Given the node map
		 | a | b | 1 | 2 | c | d |
		
		And the ways
		 | nodes |
		 | abcd  |

		When I route I should get
		 | from | to | route |
		 | 1    | 2  | abcd  |
		 | 2    | 1  | abcd  |

	Scenario: Routing between the middle nodes of way
		Given the node map
		 | a | b | c | d | e | f |

		And the ways
		 | nodes  |
		 | abcdef |

		When I route I should get
		 | from | to | route  |
		 | b    | c  | abcdef |
		 | b    | d  | abcdef |
		 | b    | e  | abcdef |
		 | c    | b  | abcdef |
		 | c    | d  | abcdef |
		 | c    | e  | abcdef |
		 | d    | b  | abcdef |
		 | d    | c  | abcdef |
		 | d    | e  | abcdef |
		 | e    | b  | abcdef |
		 | e    | c  | abcdef |
		 | e    | d  | abcdef |

	Scenario: Two ways connected in a straight line
		Given the node map
		 | a | b | c |
	
		And the ways
		 | nodes |
		 | ab    |
		 | bc    |
    
		When I route I should get
		 | from | to | route |
		 | a    | c  | ab,bc |
		 | c    | a  | bc,ab |
		 | a    | b  | ab    |
		 | b    | a  | ab    |
		 | b    | c  | bc    |
		 | c    | b  | bc    |
		
	Scenario: 2 unconnected parallel ways
		Given the node map
		 | a | b |
		 | c | d |
		
		And the ways
		 | nodes |
		 | ab    |
		 | cd    |
	    
		When I route I should get
		 | from | to | route |
		 | a    | b  | ab    |
		 | b    | a  | ab    |
		 | c    | d  | cd    |
		 | d    | c  | cd    |
		 | a    | c  |       |
		 | c    | a  |       |
		 | b    | d  |       |
		 | d    | b  |       |
		 | a    | d  |       |
		 | d    | a  |       |

	Scenario: 3 ways connected in a triangle
		Given the node map
		 | a |   | b |
		 |   |   |   |
		 |   | c |   |

		And the ways
		 | nodes |
		 | ab    |
		 | bc    |
		 | ca    |
		
		When I route I should get
		 | from | to | route |
		 | a    | b  | ab    |
		 | a    | c  | ca    |
		 | b    | c  | bc    |
		 | b    | a  | ab    |
		 | c    | a  | ca    |
		 | c    | b  | bc    |

	Scenario: To ways connected at a 45 degree angle
		Given the node map
		 | a |   |   |
		 | b |   |   |
		 | c | d | e |

		And the ways
		 | nodes |
		 | abc   |
		 | cde   |

		When I route I should get
		 | from | to | route   |
		 | b    | d  | abc,cde |
		 | a    | e  | abc,cde |
		 | a    | c  | abc     |
		 | c    | a  | abc     |
		 | c    | e  | cde     |
		 | e    | c  | cde     |


