@routing @maxspeed @car
Feature: Car - Max speed restrictions

	Background: Use specific speeds
		Given the profile "car"
		Given a grid size of 1000 meters
	
	Scenario: Car - Respect maxspeeds when lower that way type speed
		Given the node map
		 | a | b | c |

		And the ways
		 | nodes | highway  | maxspeed |
		 | ab    | trunk |          |
		 | bc    | trunk | 10       |

		When I route I should get
		 | from | to | route | time      |
		 | a    | b  | ab    | 42s ~10%  |
		 | b    | c  | bc    | 360s ~10% |

	Scenario: Car - Ignore maxspeed when higher than way speed
		Given the node map
		 | a | b | c |

		And the ways
		 | nodes | highway     | maxspeed |
		 | ab    | residential |          |
		 | bc    | residential | 85       |

		When I route I should get
		 | from | to | route | time      |
		 | a    | b  | ab    | 144s ~10% |
		 | b    | c  | bc    | 144s ~10%  |
	
	@oppposite @todo
	Scenario: Car - Forward/backward maxspeed
		Given the node map
		 | a | b | c | d | e |

		And the ways
		 | nodes | highway | maxspeed:forward | maxspeed:backward |
		 | ab    | primary |                  |                   |
		 | bc    | primart | 18               | 9                 |
		 | cd    | primart |                  | 9                 |
		 | de    | primart | 9                |                   |

		When I route I should get
		 | from | to | route | time |
		 | a    | b  | ab    | 10s  |
		 | b    | a  | ab    | 10s  |
		 | b    | c  | bc    | 20s  |
		 | c    | b  | bc    | 40s  |
		 | c    | d  | cd    | 10s  |
		 | d    | c  | cd    | 20s  |
		 | d    | e  | de    | 20s  |
		 | e    | d  | de    | 10s  |
