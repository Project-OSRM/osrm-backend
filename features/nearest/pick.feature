@nearest
Feature: Locating Nearest node on a Way - pick closest way

	Background:
		Given the profile "testbot"
	
	Scenario: Nearest - two ways crossing
		Given the node map
		 |   | 0 | c | 1 |   |
		 | 7 |   | n |   | 2 |
		 | a | k | x | m | b |
		 | 6 |   | l |   | 3 |
		 |   | 5 | d | 4 |   |

		And the ways
		 | nodes |
		 | axb   |
		 | cxd   |

		When I request nearest I should get
		 | in | out |
		 | 0  | c   |
		 | 1  | c   |
		 | 2  | b   |
		 | 3  | b   |
		 | 4  | d   |
		 | 5  | d   |
		 | 6  | a   |
		 | 7  | a   |
		 | k  | k   |
		 | l  | l   |
		 | m  | m   |
		 | n  | n   |
