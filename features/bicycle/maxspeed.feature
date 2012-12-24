@routing @maxspeed @bicycle
Feature: Bike - Max speed restrictions

	Background: Use specific speeds
		Given the profile "bicycle"
	
	Scenario: Bike - Respect maxspeeds when lower that way type speed
		Given the node map
		 | a | b | c |

		And the ways
		 | nodes | highway     | maxspeed |
		 | ab    | residential |          |
		 | bc    | residential | 10       |

		When I route I should get
		 | from | to | route | time    |
		 | a    | b  | ab    | 20s ~5% |
		 | b    | c  | bc    | 36s ~5% |

	Scenario: Bike - Do not use maxspeed when higher that way type speed
		Given the node map
		 | a | b | c |

		And the ways
		 | nodes | highway     | maxspeed |
		 | ab    | residential |          |
		 | bc    | residential | 80       |

		When I route I should get
		 | from | to | route | time    |
		 | a    | b  | ab    | 20s ~5% |
		 | b    | c  | bc    | 20s ~5% |
