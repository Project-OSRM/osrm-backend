@routing @testbot @ferry
Feature: Testbot - Handle ferry routes

	Background:
		Given the speedprofile "testbot"

	Scenario: Testbot - Ferry duration, single node
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

	Scenario: Testbot - Ferry duration, multiple nodes
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