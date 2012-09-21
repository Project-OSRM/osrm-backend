@routing @distance
Feature: Distance calculation
	
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
		 | a    | b  | abc   | 1 +-1    |
		 | b    | a  | abc   | 1 +-1    |
		 | b    | c  | abc   | 1 +-1    |
		 | c    | b  | abc   | 1 +-1    |
		 | a    | c  | abc   | 2 +-1    |
		 | c    | a  | abc   | 2 +-1    |

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
		 | a    | b  | abc   | 10 +-1   |
		 | b    | a  | abc   | 10 +-1   |
		 | b    | c  | abc   | 10 +-1   |
		 | c    | b  | abc   | 10 +-1   |
		 | a    | c  | abc   | 20 +-1   |
		 | c    | a  | abc   | 20 +-1   |

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
		 | a    | b  | abc   | 100 +-1  |
		 | b    | a  | abc   | 100 +-1  |
		 | b    | c  | abc   | 100 +-1  |
		 | c    | b  | abc   | 100 +-1  |
		 | a    | c  | abc   | 200 +-1  |
		 | c    | a  | abc   | 200 +-1  |

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
		 | a    | b  | abc   | 1000 +-1 |
		 | b    | a  | abc   | 1000 +-1 |
		 | b    | c  | abc   | 1000 +-1 |
		 | c    | b  | abc   | 1000 +-1 |
		 | a    | c  | abc   | 2000 +-1 |
		 | c    | a  | abc   | 2000 +-1 |

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
		 | a    | b  | abcdefgh | 10 +-1   |
		 | a    | c  | abcdefgh | 20 +-1   |
		 | a    | d  | abcdefgh | 30 +-1   |
		 | a    | e  | abcdefgh | 40 +-1   |
		 | a    | f  | abcdefgh | 50 +-1   |
		 | a    | g  | abcdefgh | 60 +-1   |
		 | a    | h  | abcdefgh | 70 +-1   |
		
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
		 | a    | b  | abcdefgh | 10 +-1   |
		 | a    | c  | abcdefgh | 20 +-1   |
		 | a    | d  | abcdefgh | 30 +-1   |
		 | a    | e  | abcdefgh | 40 +-1   |
		 | a    | f  | abcdefgh | 50 +-1   |
		 | a    | g  | abcdefgh | 60 +-1   |
		 | a    | h  | abcdefgh | 70 +-1   |

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
		 | x    | a  | xa    | 3000 +-1 |
		 | x    | b  | xb    | 3160 +-1 |
		 | x    | c  | xc    | 3610 +-1 |
		 | x    | d  | xd    | 4240 +-1 |
		 | x    | e  | xe    | 3610 +-1 |
		 | x    | f  | xf    | 3160 +-1 |
		 | x    | g  | xg    | 3000 +-1 |
		 | x    | h  | xh    | 3160 +-1 |
		 | x    | i  | xi    | 3610 +-1 |
		 | x    | j  | xj    | 4240 +-1 |
		 | x    | k  | xk    | 3610 +-1 |
		 | x    | l  | xl    | 3160 +-1 |
		 | x    | m  | xm    | 3000 +-1 |
		 | x    | n  | xn    | 3160 +-1 |
		 | x    | o  | xo    | 3610 +-1 |
		 | x    | p  | xp    | 4240 +-1 |
		 | x    | q  | xq    | 3610 +-1 |
		 | x    | r  | xr    | 3160 +-1 |
		 | x    | s  | xs    | 3000 +-1 |
		 | x    | t  | xt    | 3160 +-1 |
		 | x    | u  | xu    | 3610 +-1 |
		 | x    | v  | xv    | 4240 +-1 |
		 | x    | w  | xw    | 3610 +-1 |
		 | x    | y  | xy    | 3160 +-1 |