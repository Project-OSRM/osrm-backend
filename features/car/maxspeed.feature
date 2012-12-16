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
