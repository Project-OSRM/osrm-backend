@routing @maxspeed
Feature: Max speed restrictions

	Background: Use specific speeds
		Given the speedprofile "car"
		Given a grid size of 1000 meters
	
	Scenario: Car -  Maxspeed below profile speed of way type
		Given the node map
		 | a | b | c |

		And the ways
		 | nodes | highway  | maxspeed |
		 | ab    | motorway |          |
		 | bc    | motorway | 10       |

		When I route I should get
		 | from | to | route | time      |
		 | a    | b  | ab    | 40s ~10%  |
		 | b    | c  | bc    | 360s ~10% |

	Scenario: Car -  Maxspeed above profile speed of way type
		Given the node map
		 | a | b | c |

		And the ways
		 | nodes | highway     | maxspeed |
		 | ab    | residential |          |
		 | bc    | residential | 50       |

		When I route I should get
		 | from | to | route | time      |
		 | a    | b  | ab    | 145s ~10% |
		 | b    | c  | bc    | 145s ~10% |
