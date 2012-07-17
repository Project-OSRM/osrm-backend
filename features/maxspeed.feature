@routing @maxspeed
Feature: Speed limits
	Note:
	60km/h = 100m/6s
	30km/h = 100m/12s
	15km/h = 100m/24s
	10km/h = 100m/48s
	5km/h  = 100m/72s
	
	Scenario: Obey speedlimits
		Given the speedprofile "car"
		And the speedprofile settings
		 | primary | 60 |
		And a grid size of 100 meters
		And the node map
		 | a | b |
		 | c | d |

		And the ways
		 | nodes | highway | maxspeed |
		 | ab    | primary |          |
		 | cd    | primary | 30       |

		When I route I should get
		 | from | to | route | time |
		 | a    | b  | ab    | 6s   |
		 | c    | d  | cd    | 12s  |

	Scenario: Go faster than speedprofile when takeMinimumOfSpeeds=no
		Given the speedprofile "car"
		And the speedprofile settings
		 | residential         | 15 |
		 | takeMinimumOfSpeeds | no |
		And a grid size of 100 meters
		And the node map
		 | a | b |

		And the ways
		 | nodes | highway     | maxspeed |
		 | ab    | residential | 30       |

		When I route I should get
		 | from | to | route | time |
		 | a    | b  | ab    | 12s  |

	Scenario: Bicycles can't go faster just because maxspeed is high
		Given the speedprofile "bicycle"
		And the speedprofile settings
		 | primary | 15 |
		And a grid size of 100 meters
		
		And the node map
		 | a | b |
		 | c | d |
		
		And the ways
		 | nodes | highway | maxspeed |
		 | ab    | primary |          |
		 | cd    | primary | 60       |

		When I route I should get
		 | from | to | route | time |
		 | a    | b  | ab    | 24s  |
		 | c    | d  | cd    | 24s  |

	Scenario: Bicycles should also obey maxspeed
		Given the speedprofile "bicycle"
		And the speedprofile settings
		 | primary | 15 |
		And a grid size of 100 meters

		And the node map
		 | a | b |
		 | c | d |

		And the ways
		 | nodes | highway | maxspeed |
		 | ab    | primary |          |
		 | cd    | primary | 10       |

		When I route I should get
		 | from | to | route | time |
		 | a    | b  | ab    | 24s  |
		 | c    | d  | cd    | 48s  |


