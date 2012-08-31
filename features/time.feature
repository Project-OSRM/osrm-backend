@routing @time
Feature: Estimation of travel time
	Note:
	15km/h = 15000m/3600s = 150m/36s = 100m/24s
	5km/h = 5000m/3600s = 50m/36s = 100m/72s
	
	Going dioganally causes the distance to increase by sqrt(2):
	24s * sqrt(2)=  33.941125496954285s
	
	Background: Use specific speeds
		Given the speedprofile "bicycle"
	
	@basic_time
	Scenario: Basic travel time, 1m scale
		Given a grid size of 1 meters
		Given the node map
		 | h | a | b |
		 | g | x | c |
		 | f | e | d |

		And the ways
		 | nodes | highway   |
		 | xa    | primary   |
		 | xb    | primary   |
		 | xc    | primary   |
		 | xd    | primary   |
		 | xe    | primary   |
		 | xf    | primary   |
		 | xg    | primary   |
		 | xh    | primary   |

		When I route I should get
		 | from | to | route | time |
		 | x    | a  | xa    | 0s  |
		 | x    | b  | xb    | 0s  |
		 | x    | c  | xc    | 0s  |
		 | x    | d  | xd    | 0s  |
		 | x    | e  | xe    | 0s  |
		 | x    | f  | xf    | 0s  |
		 | x    | g  | xg    | 0s  |
		 | x    | h  | xh    | 0s  |
	
	@basic_time
	Scenario: Basic travel time, 10m scale
		Given a grid size of 10 meters
		Given the node map
		 | h | a | b |
		 | g | x | c |
		 | f | e | d |

		And the ways
		 | nodes | highway   |
		 | xa    | primary   |
		 | xb    | primary   |
		 | xc    | primary   |
		 | xd    | primary   |
		 | xe    | primary   |
		 | xf    | primary   |
		 | xg    | primary   |
		 | xh    | primary   |

		When I route I should get
		 | from | to | route | time |
		 | x    | a  | xa    | 2s  |
		 | x    | b  | xb    | 3s  |
		 | x    | c  | xc    | 2s  |
		 | x    | d  | xd    | 3s  |
		 | x    | e  | xe    | 2s  |
		 | x    | f  | xf    | 3s  |
		 | x    | g  | xg    | 2s  |
		 | x    | h  | xh    | 3s  |

	@basic_time
	Scenario: Basic travel time, 100m scale
		Given a grid size of 100 meters
		Given the node map
		 | h | a | b |
		 | g | x | c |
		 | f | e | d |

		And the ways
		 | nodes | highway   |
		 | xa    | primary   |
		 | xb    | primary   |
		 | xc    | primary   |
		 | xd    | primary   |
		 | xe    | primary   |
		 | xf    | primary   |
		 | xg    | primary   |
		 | xh    | primary   |
	
		When I route I should get
		 | from | to | route | time |
		 | x    | a  | xa    | 24s  |
		 | x    | b  | xb    | 34s  |
		 | x    | c  | xc    | 24s  |
		 | x    | d  | xd    | 34s  |
		 | x    | e  | xe    | 24s  |
		 | x    | f  | xf    | 34s  |
		 | x    | g  | xg    | 24s  |
		 | x    | h  | xh    | 34s  |

	@basic_time
	Scenario: Basic travel time, 1km scale
		Given a grid size of 1000 meters
		Given the node map
		 | h | a | b |
		 | g | x | c |
		 | f | e | d |

		And the ways
		 | nodes | highway   |
		 | xa    | primary   |
		 | xb    | primary   |
		 | xc    | primary   |
		 | xd    | primary   |
		 | xe    | primary   |
		 | xf    | primary   |
		 | xg    | primary   |
		 | xh    | primary   |

		When I route I should get
		 | from | to | route | time |
		 | x    | a  | xa    | 240s  |
		 | x    | b  | xb    | 339s  |
		 | x    | c  | xc    | 240s  |
		 | x    | d  | xd    | 339s  |
		 | x    | e  | xe    | 240s  |
		 | x    | f  | xf    | 339s  |
		 | x    | g  | xg    | 240s  |
		 | x    | h  | xh    | 339s  |

	@basic_time
	Scenario: Basic travel time, 10km scale
		Given a grid size of 10000 meters
		Given the node map
		 | h | a | b |
		 | g | x | c |
		 | f | e | d |
		
		And the ways
		 | nodes | highway   |
		 | xa    | primary   |
		 | xb    | primary   |
		 | xc    | primary   |
		 | xd    | primary   |
		 | xe    | primary   |
		 | xf    | primary   |
		 | xg    | primary   |
		 | xh    | primary   |
		
		When I route I should get
		 | from | to | route | time |
		 | x    | a  | xa    | 2400s  |
		 | x    | b  | xb    | 3394s  |
		 | x    | c  | xc    | 2400s  |
		 | x    | d  | xd    | 3394s  |
		 | x    | e  | xe    | 2400s  |
		 | x    | f  | xf    | 3394s  |
		 | x    | g  | xg    | 2400s  |
		 | x    | h  | xh    | 3394s  |

	@basic_time
	Scenario: Basic travel time, 100km scale
		Given a grid size of 100000 meters
		Given the node map
		 | h | a | b |
		 | g | x | c |
		 | f | e | d |

		And the ways
		 | nodes | highway   |
		 | xa    | primary   |
		 | xb    | primary   |
		 | xc    | primary   |
		 | xd    | primary   |
		 | xe    | primary   |
		 | xf    | primary   |
		 | xg    | primary   |
		 | xh    | primary   |

		When I route I should get
		 | from | to | route | time |
		 | x    | a  | xa    | 24000s  |
		 | x    | b  | xb    | 33941s  |
		 | x    | c  | xc    | 24000s  |
		 | x    | d  | xd    | 33941s  |
		 | x    | e  | xe    | 24000s  |
		 | x    | f  | xf    | 33941s  |
		 | x    | g  | xg    | 24000s  |
		 | x    | h  | xh    | 33941s  |

	Scenario: Time of travel depending on way type
		Given the node map
		 | a | b |
		 | c | d |

		And the ways
		 | nodes | highway   |
		 | ab    | primary   |
		 | cd    | footway   |
		
		When I route I should get
		 | from | to | route | time |
		 | a    | b  | ab    | 24s  |
		 | c    | d  | cd    | 72s  |

	Scenario: Time of travel on a series of ways
		Given the node map
		 | a | b | c | d |

		And the ways
		 | nodes | highway |
		 | ab    | primary |
		 | bc    | primary |
		 | cd    | primary |

		When I route I should get
		 | from | to | route    | time |
		 | a    | b  | ab       | 24s  |
		 | a    | c  | ab,bc    | 48s  |
		 | a    | d  | ab,bc,cd | 72s  |

	Scenario: Time of travel on a winding way
		Given the node map
		 | a | b |   |   |   |   |
		 |   | c | d | e |   | i |
		 |   |   |   | f | g | h |

		And the ways
		 | nodes     | highway |
		 | abcdefghi | primary |

		When I route I should get
		 | from | to | route     | time |
		 | a    | b  | abcdefghi | 24s  |
		 | a    | e  | abcdefghi | 96s  |
		 | a    | i  | abcdefghi | 192s |

	Scenario: Time of travel on combination of road types
		Given the node map
		 | a | b | c | d | e |

		And the ways
		 | nodes | highway   |
		 | abc   | primary   |
		 | cde   | footway   |

		When I route I should get
		 | from | to | route   | time |
		 | b    | c  | abc     | 24s  |
		 | c    | e  | cde     | 72s  |
		 | b    | d  | abc,cde | 144s |
		 | a    | e  | abc,cde | 192s |

	Scenario: Time of travel on part of a way
		Given the node map
		 | a | 1 |
		 |   | 2 |
		 |   | 3 |
		 | b | 4 |

		And the ways
		 | nodes | highway |
		 | ab    | primary |

		When I route I should get
		 | from | to | route | time |
		 | 1    | 2  | ab    | 24s  |
		 | 1    | 3  | ab    | 48s  |
		 | 1    | 4  | ab    | 72s  |
		 | 4    | 3  | ab    | 24s  |
		 | 4    | 2  | ab    | 48s  |
		 | 4    | 1  | ab    | 72s  |