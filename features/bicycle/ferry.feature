@routing @bicycle @ferry
Feature: Bike - Handle ferry routes

	Background:
		Given the profile "bicycle"
	
	Scenario: Bike - Ferry route
		Given the node map
		 | a | b | c |   |   |
		 |   |   | d |   |   |
		 |   |   | e | f | g |
	
		And the ways
		 | nodes | highway | route | bicycle |
		 | abc   | primary |       |         |
		 | cde   |         | ferry | yes     |
		 | efg   | primary |       |         |
   
		When I route I should get
		 | from | to | route       |
		 | a    | g  | abc,cde,efg |
		 | b    | f  | abc,cde,efg |
		 | e    | c  | cde         |
		 | e    | b  | cde,abc     |
		 | e    | a  | cde,abc     |
		 | c    | e  | cde         |
		 | c    | f  | cde,efg     |
		 | c    | g  | cde,efg     |

	Scenario: Bike - Ferry duration, single node
		Given the node map
		 | a | b | c | d |
		 |   |   | e | f |
		 |   |   | g | h |
		 |   |   | i | j |
		 
		And the ways
		 | nodes | highway | route | bicycle | duration |
		 | ab    | primary |       |         |          |
		 | cd    | primary |       |         |          |
		 | ef    | primary |       |         |          |
		 | gh    | primary |       |         |          |
		 | ij    | primary |       |         |          |
		 | bc    |         | ferry | yes     | 0:01     |
		 | be    |         | ferry | yes     | 0:10     |
		 | bg    |         | ferry | yes     | 1:00     |
		 | bi    |         | ferry | yes     | 10:00    |

	Scenario: Bike - Ferry duration, multiple nodes
		Given the node map
		  | x |   |   |   |   | y |
		  |   | a | b | c | d |   |

		And the ways
		 | nodes | highway | route | bicycle | duration |
		 | xa    | primary |       |         |          |
		 | yd    | primary |       |         |          |
		 | abcd  |         | ferry | yes     | 1:00     |

		When I route I should get
		 | from | to | route | time      |
		 | a    | d  | abcd  | 3600s +-1 |
		 | d    | a  | abcd  | 3600s +-1 |

	Scenario: Bike - Ferry duration, connected routes
		Given the node map
		  | x |   |   |   |   |   |   |   | y |
		  |   | a | b | c | d | e | f | g |   |

		And the ways
		 | nodes | highway | route | bicycle | duration |
		 | xa    | primary |       |         |          |
		 | yg    | primary |       |         |          |
		 | abcd  |         | ferry | yes     | 0:30     |
		 | defg  |         | ferry | yes     | 0:30     |

		When I route I should get
		 | from | to | route     | time      |
		 | a    | g  | abcd,defg | 3600s +-1 |
		 | g    | a  | defg,abcd | 3600s +-1 |

	Scenario: Bike - Prefer road when faster than ferry
		Given the node map
		  | x | a | b | c |   |
		  |   |   |   |   | d |
		  | y | g | f | e |   |
		
		And the ways
		 | nodes | highway | route | bicycle | duration |
		 | xa    | primary |       |         |          |
		 | yg    | primary |       |         |          |
		 | xy    | primary |       |         |          |
		 | abcd  |         | ferry | yes     | 0:01     |
		 | defg  |         | ferry | yes     | 0:01     |

		When I route I should get
		 | from | to | route    | time      |
		 | a    | g  | xa,xy,yg | 60s +-25% |
		 | g    | a  | yg,xy,xa | 60s +-25% |

	Scenario: Bike - Long winding ferry route
		Given the node map
		  | x |   | b |   | d |   | f |   | y |
		  |   | a |   | c |   | e |   | g |   |

		And the ways
		 | nodes   | highway | route | bicycle | duration |
		 | xa      | primary |       |         |          |
		 | yg      | primary |       |         |          |
		 | abcdefg |         | ferry | yes     | 6:30     |

		When I route I should get
		 | from | to | route   | time       |
		 | a    | g  | abcdefg | 23400s +-1 |
		 | g    | a  | abcdefg | 23400s +-1 |
    
    @todo
	Scenario: Bike - Ferry duration, individual parts
		Given the node map
		  | x | y |  | z |  |  | v |
		  | a | b |  | c |  |  | d |

		And the ways
		 | nodes | highway | route | bicycle | duration |
		 | xa    | primary |       |         |          |
		 | yb    | primary |       |         |          |
		 | zc    | primary |       |         |          |
		 | vd    | primary |       |         |          |
		 | abcd  |         | ferry | yes     | 1:00     |

		When I route I should get
		 | from | to | route | time      |
		 | a    | d  | abcd  | 3600s +-1 |
		 | a    | b  | abcd  | 600s +-1  |
		 | b    | c  | abcd  | 1200s +-1 |
		 | c    | d  | abcd  | 1800s +-1 |
