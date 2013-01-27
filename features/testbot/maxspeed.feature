@routing @maxspeed @testbot
Feature: Car - Max speed restrictions

	Background: Use specific speeds
		Given the profile "testbot"
	
	Scenario: Testbot - Respect maxspeeds when lower that way type speed
		Given the node map
		 | a | b | c | d |

		And the ways
		 | nodes | maxspeed |
		 | ab    |          |
		 | bc    | 24       |
		 | cd    | 18       |

		When I route I should get
		 | from | to | route | time    |
		 | a    | b  | ab    | 10s +-1 |
		 | b    | a  | ab    | 10s +-1 |
		 | b    | c  | bc    | 15s +-1 |
		 | c    | b  | bc    | 15s +-1 |
		 | c    | d  | cd    | 20s +-1 |
		 | d    | c  | cd    | 20s +-1 |

	Scenario: Testbot - Ignore maxspeed when higher than way speed
		Given the node map
		 | a | b | c |

		And the ways
		 | nodes | maxspeed |
		 | ab    |          |
		 | bc    | 200      |

		When I route I should get
		 | from | to | route | time    |
		 | a    | b  | ab    | 10s +-1 |
		 | b    | a  | ab    | 10s +-1 |
		 | b    | c  | bc    | 10s +-1 |
		 | c    | b  | bc    | 10s +-1 |

    @opposite
    Scenario: Testbot - Forward/backward maxspeed
    	Given the node map
    	 | a | b | c | d | e | f | g | h |

    	And the ways
    	 | nodes | maxspeed | maxspeed:forward | maxspeed:backward |
    	 | ab    |          |                  |                   |
    	 | bc    | 18       |                  |                   |
    	 | cd    |          | 18               |                   |
    	 | de    |          |                  | 18                |
    	 | ef    | 9        | 18               |                   |
    	 | fg    | 9        |                  | 18                |
    	 | gh    | 9        | 24               | 18                |

    	When I route I should get
    	 | from | to | route | time    |
    	 | a    | b  | ab    | 10s +-1 |
    	 | b    | a  | ab    | 10s +-1 |
    	 | b    | c  | bc    | 20s +-1 |
    	 | c    | b  | bc    | 20s +-1 |
    	 | c    | d  | cd    | 20s +-1 |
    	 | d    | c  | cd    | 10s +-1 |
    	 | d    | e  | de    | 10s +-1 |
    	 | e    | d  | de    | 20s +-1 |
    	 | e    | f  | ef    | 20s +-1 |
    	 | f    | e  | ef    | 40s +-1 |
    	 | f    | g  | fg    | 40s +-1 |
    	 | g    | f  | fg    | 20s +-1 |
    	 | g    | h  | gh    | 15s +-1 |
    	 | h    | g  | gh    | 20s +-1 |
