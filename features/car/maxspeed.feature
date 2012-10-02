@routing @maxspeed
Feature: Max speed restrictions

	Background: Use specific speeds
		Given the speedprofile "car"
		Given a grid size of 1000 meters
	
	Scenario: Car -  Max speed on a fast road
		Given the node map
		 | a | b | c |

		And the ways
		 | nodes | highway  | maxspeed |
		 | ab    | motorway |          |
		 | bc    | motorway | 10       |

		When I route I should get
		 | from | to | route | time     |
		 | a    | b  | ab    | 60s ~10% |
		 | b    | c  | bc    | 360s +-1 |

	Scenario: Car -  Max speed on a slow roads
		Given the node map
		 | a | b | c |

		And the ways
		 | nodes | maxspeed |
		 | ab    |          |
		 | bc    | 10       |

		When I route I should get
		 | from | to | route | time     |
		 | a    | b  | ab    | 60s ~10% |
		 | b    | c  | bc    | 360s +-1 |
