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

 	Scenario: Nearest - inside a triangle
 		Given the node map
		 |   |  |  |   |   | c |   |   |  |  |   |
		 |   |  |  |   |   |   |   |   |  |  |   |
		 |   |  |  | y |   |   |   | z |  |  |   |
		 |   |  |  |   | 0 |   | 1 |   |  |  |   |
		 |   |  |  | 2 |   | 3 |   | 4 |  |  |   |
		 | a |  |  | x |   | u |   | w |  |  | b |

 		And the ways
 		 | nodes |
 		 | ab    |
 		 | bc    |
 		 | ca    |

 		When I request nearest I should get
 		 | in | out |
 		 | 0  | y   |
 		 | 1  | z   |
 		 | 2  | x   |
 		 | 3  | u   |
 		 | 4  | w   |

  	Scenario: Nearest - only pick routable ways
  		Given the node map
 		 |   | a | c | e |   |
 		 | 0 | z | y | x | 1 |
 		 |   | b | d | f |   |

  		And the ways
  		 | nodes | highway | barrier |
  		 | ab    |         | wall    |
  		 | cd    | (nil)   |         |
  		 | ef    | primary |         |

  		When I request nearest I should get
  		 | in | out |
  		 | 0  | z   |
  		 | 1  | x   |
