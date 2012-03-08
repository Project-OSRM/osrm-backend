@routing @distance
Feature: Distance calculation

	Scenario: Distance of a winding south-north path
		Given a grid size of 10 meters
		Given the node map
		 | a | b |
		 | d | c |
		 | e | f |
		 | h | g |

		And the ways
		 | nodes    |
		 | abcdefgh |

		When I route I should get
		 | from | to | route    | distance |
		 | a    | b  | abcdefgh | 10       |
		 | a    | c  | abcdefgh | 20       |
		 | a    | d  | abcdefgh | 30       |
		 | a    | e  | abcdefgh | 40       |
		 | a    | f  | abcdefgh | 50       |
		 | a    | g  | abcdefgh | 60       |
		 | a    | h  | abcdefgh | 70       |
		
	Scenario: Distance of a winding east-west path
		Given a grid size of 10 meters
		Given the node map
		 | a | d | e | h |
		 | b | c | f | g |

		And the ways
		 | nodes    |
		 | abcdefgh |

		When I route I should get
		 | from | to | route    | distance |
		 | a    | b  | abcdefgh | 10       |
		 | a    | c  | abcdefgh | 20       |
		 | a    | d  | abcdefgh | 30       |
		 | a    | e  | abcdefgh | 40       |
		 | a    | f  | abcdefgh | 50       |
		 | a    | g  | abcdefgh | 60       |
		 | a    | h  | abcdefgh | 70       |

	Scenario: Distances when traversing part of a way
		Given a grid size of 100 meters
		Given the node map
		 | a | 0 | 1 | 2 |
		 | 9 |   |   | 3 |
		 | 8 |   |   | 4 |
		 | 7 | 6 | 5 | b |

		And the ways
		 | nodes |
		 | ab    |

		When I route I should get
		 | from | to | route | distance |
		 | a    | 0  | ab    | 70       |
		 | a    | 1  | ab    | 140      |
		 | a    | 2  | ab    | 210      |
		 | a    | 3  | ab    | 280      |
		 | a    | 4  | ab    | 350      |
		 | a    | b  | ab    | 420      |
		 | a    | 5  | ab    | 350      |
		 | a    | 6  | ab    | 280      |
		 | a    | 7  | ab    | 210      |
		 | a    | 8  | ab    | 140      |
		 | a    | 9  | ab    | 70       |
		 | b    | 5  | ab    | 70       |
		 | b    | 6  | ab    | 140      |
		 | b    | 7  | ab    | 210      |
		 | b    | 8  | ab    | 280      |
		 | b    | 9  | ab    | 350      |
		 | b    | a  | ab    | 420      |
		 | b    | 0  | ab    | 350      |
		 | b    | 1  | ab    | 280      |
		 | b    | 2  | ab    | 210      |
		 | b    | 3  | ab    | 140      |
		 | b    | 4  | ab    | 70       |

	Scenario: Geometric distances
		Given a grid size of 1000 meters
		Given the node map
		 | v | w | y | a | b | c | d |
		 | u |   |   |   |   |   | e |
		 | t |   |   |   |   |   | f |
		 | s |   |   | x |   |   | g |
		 | r |   |   |   |   |   | h |
		 | q |   |   |   |   |   | i |
		 | p | o | n | m | l | k | j |
		
		And the ways
		 | nodes |
		 | xa    |
		 | xb    |
		 | xc    |
		 | xd    |
		 | xe    |
		 | xf    |
		 | xg    |
		 | xh    |
		 | xi    |
		 | xj    |
		 | xk    |
		 | xl    |
		 | xm    |
		 | xn    |
		 | xo    |
		 | xp    |
		 | xq    |
		 | xr    |
		 | xs    |
		 | xt    |
		 | xu    |
		 | xv    |
		 | xw    |
		 | xy    |

		When I route I should get
		 | from | to | route | distance |
		 | x    | a  | xa    | 3000     |
		 | x    | b  | xb    | 3160     |
		 | x    | c  | xc    | 3610     |
		 | x    | d  | xd    | 4240     |
		 | x    | e  | xe    | 3610     |
		 | x    | f  | xf    | 3160     |
		 | x    | g  | xg    | 3000     |
		 | x    | h  | xh    | 3160     |
		 | x    | i  | xi    | 3610     |
		 | x    | j  | xj    | 4240     |
		 | x    | k  | xk    | 3610     |
		 | x    | l  | xl    | 3160     |
		 | x    | m  | xm    | 3000     |
		 | x    | n  | xn    | 3160     |
		 | x    | o  | xo    | 3610     |
		 | x    | p  | xp    | 4240     |
		 | x    | q  | xq    | 3610     |
		 | x    | r  | xr    | 3160     |
		 | x    | s  | xs    | 3000     |
		 | x    | t  | xt    | 3160     |
		 | x    | u  | xu    | 3610     |
		 | x    | v  | xv    | 4240     |
		 | x    | w  | xw    | 3610     |
		 | x    | y  | xy    | 3160     |

	Scenario: 1m distances
		Given a grid size of 1 meters
		Given the node map
		 | a | b |
		 |   | c |

		And the ways
		 | nodes |
		 | abc   |

		When I route I should get
		 | from | to | route | distance |
		 | a    | b  | abc   | 1        |
		 | b    | a  | abc   | 1        |
		 | b    | c  | abc   | 1        |
		 | c    | b  | abc   | 1        |
		 | a    | c  | abc   | 2        |
		 | c    | a  | abc   | 2        |

	Scenario: 10m distances
		Given a grid size of 10 meters
		Given the node map
		 | a | b |
		 |   | c |

		And the ways
		 | nodes |
		 | abc   |

		When I route I should get
		 | from | to | route | distance |
		 | a    | b  | abc   | 10       |
		 | b    | a  | abc   | 10       |
		 | b    | c  | abc   | 10       |
		 | c    | b  | abc   | 10       |
		 | a    | c  | abc   | 20       |
		 | c    | a  | abc   | 20       |

	Scenario: 100m distances
		Given a grid size of 100 meters
		Given the node map
		 | a | b |
		 |   | c |

		And the ways
		 | nodes |
		 | abc   |

		When I route I should get
		 | from | to | route | distance |
		 | a    | b  | abc   | 100      |
		 | b    | a  | abc   | 100      |
		 | b    | c  | abc   | 100      |
		 | c    | b  | abc   | 100      |
		 | a    | c  | abc   | 200      |
		 | c    | a  | abc   | 200      |

	Scenario: 1km distance
		Given a grid size of 1000 meters
		Given the node map
		 | a | b |
		 |   | c |

		And the ways
		 | nodes |
		 | abc   |

		When I route I should get
		 | from | to | route | distance |
		 | a    | b  | abc   | 1000     |
		 | b    | a  | abc   | 1000     |
		 | b    | c  | abc   | 1000     |
		 | c    | b  | abc   | 1000     |
		 | a    | c  | abc   | 2000     |
		 | c    | a  | abc   | 2000     |

	Scenario: 10km distances
		Given a grid size of 10000 meters
		Given the node map
		 | a | b |
		 |   | c |

		And the ways
		 | nodes |
		 | abc   |

		When I route I should get
		 | from | to | route | distance |
		 | a    | b  | abc   | 10000    |
		 | b    | a  | abc   | 10000    |
		 | b    | c  | abc   | 10000    |
		 | c    | b  | abc   | 10000    |
		 | a    | c  | abc   | 20000    |
		 | c    | a  | abc   | 20000    |

	Scenario: 100km distances
		Given a grid size of 100000 meters
		Given the node map
		 | a | b |
		 |   | c |

		And the ways
		 | nodes |
		 | abc   |

		When I route I should get
		 | from | to | route | distance |
		 | a    | b  | abc   | 100000   |
		 | b    | a  | abc   | 100000   |
		 | b    | c  | abc   | 100000   |
		 | c    | b  | abc   | 100000   |
		 | a    | c  | abc   | 200000   |
		 | c    | a  | abc   | 200000   |

	Scenario: 1000km distances
		Given a grid size of 1000000 meters
		Given the node map
		 | a | b |
		 |   | c |

		And the ways
		 | nodes |
		 | abc   |

		When I route I should get
		 | from | to | route | distance |
		 | a    | b  | abc   | 1000000  |
		 | b    | a  | abc   | 1000000  |
		 | b    | c  | abc   | 1000000  |
		 | c    | b  | abc   | 1000000  |
		 | a    | c  | abc   | 2000000  |
		 | c    | a  | abc   | 2000000  |
		
	Scenario: Angles at 1000km scale
		Given a grid size of 1000 meters
		Given the node map
		 |   |   |   | b |   |   |   |
		 |   |   |   |   |   |   | c |
		 | a |   |   |   |   |   |   |
		 |   |   |   | e |   |   |   |
		 |   |   |   |   |   |   | f |
		 | d |   |   |   |   |   |   |

		And the ways
		 | nodes |
		 | ba    |
		 | bc    |
		 | ed    |
		 | ef    |

		When I route I should get
		 | from | to | route | distance |
		 | b    | c  | bc    | 3160     |
		 | e    | f  | ef    | 3160     |