@routing @snap
Feature: Snap start/end point to the nearest way 
	
	Scenario: Snap to nearest protruding oneway
		Given the nodes
		 |   | 1 |   | 2 |   |
		 | 8 |   | n |   | 3 |
		 |   | w | c | e |   |
		 | 7 |   | s |   | 4 |
		 |   | 6 |   | 5 |   |

		And the ways
		 | nodes |
		 | nc    |
		 | ec    |
		 | sc    |
		 | wc    |

		When I route I should get
		 | from | to | route |
		 | 1    | c  | nc    |
		 | 2    | c  | nc    |
		 | 3    | c  | ec    |
		 | 4    | c  | ec    |
		 | 5    | c  | sc    |
		 | 6    | c  | sc    |
		 | 7    | c  | wc    |
		 | 8    | c  | wc    |
	
	Scenario: Snap to nearest edge of a square
		Given the nodes
		 | 4 | 5 | 6 | 7 |
		 | 3 | a |   | u |
		 | 2 |   |   |   |
		 | 1 | d |   | b |

		And the ways
		 | nodes |
		 | aub   |
		 | adb   |

		When I route I should get
		 | from | to | route |
		 | 1    | b  | adb   |
		 | 2    | b  | adb   |
		 | 6    | b  | aub   |
		 | 7    | b  | aub   |

	Scenario: Snap to edge right under start/end point
		Given the nodes
		 | d | e | f | g |
		 | c |   |   | h |
		 | b |   |   | i |
		 | a | l | k | j |

		And the ways
		 | nodes |
		 | abcd  |
		 | defg  |
		 | ghij  |
		 | jkla  |

		When I route I should get
		 | from | to | route |
		 | a    | b  | abcd  |
		 | a    | c  | abcd  |
		 | a    | d  | abcd  |
		 | a    | e  | abcd  |
		 | a    | f  | abcd  |
		 | a    | g  | abcd  |
		 | a    | h  | jkla  |
		 | a    | i  | jkla  |
		 | a    | j  | jkla  |
		 | a    | k  | jkla  |
		 | a    | l  | jkla  |
		 | b    | a  | abcd  |
		 | b    | c  | abcd  |
		 | b    | d  | abcd  |
		 | b    | e  | abcd  |
		 | b    | f  | abcd  |
		 | b    | g  | abcd  |
		 | b    | h  | jkla  |
		 | b    | i  | jkla  |
		 | b    | j  | jkla  |
		 | b    | k  | jkla  |
		 | b    | l  | jkla  |
