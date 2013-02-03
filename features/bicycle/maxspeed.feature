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

     @todo
     Scenario: Bike - Forward/backward maxspeed
     	Given the node map
     	 | a | b | c | d | e | f | g | h |

        And the shortcuts
 		 | key   | value    |
 		 | bike  | 43s ~10% |
 		 | run   | 73s ~10% |
 		 | walk  | 170s ~10% |
 		 | snail | 720s ~10% |

  	 	Then routability should be
     	 | maxspeed | maxspeed:forward | maxspeed:backward | forw  | backw |
     	 |          |                  |                   | bike  | bike   |
     	 | 10       |                  |                   | run   | run    |
     	 |          | 10               |                   | run   | bike   |
     	 |          |                  | 10                | bike  | run    |
     	 | 1        | 10               |                   | run   | snail  |
     	 | 1        |                  | 10                | snail | run    |
     	 | 1        | 5                | 10                | walk  | run    |
