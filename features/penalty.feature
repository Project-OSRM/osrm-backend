@routing @penalty @signal
Feature: Penalties
	
	Background:
		Given the speedprofile "testbot"
			
	Scenario: Passing a traffic signal should incur a delay
		Given the node map
		 | a | b | c |
		 | d | e | f |

		And the nodes
		 | node | highway         |
		 | e    | traffic_signals |

		And the ways
		 | nodes |
		 | abc   |
		 | def   |

		When I route I should get
		 | from | to | route | time    |
		 | a    | c  | abc   | 20s +-1 |
		 | d    | f  | def   | 50s +-1 |

	Scenario: Passing multiple traffic signals should incur a accumulated delay
		Given the node map
		 | a | b | c | d | e |

		And the nodes
		 | node | highway         |
		 | b    | traffic_signals |
		 | c    | traffic_signals |
		 | d    | traffic_signals |

		And the ways
		 | nodes  |
		 | abcde |

		When I route I should get
		 | from | to | route | time     |
		 | a    | e  | abcde | 130s +-1 |

	Scenario: Starting or ending at a traffic signal should not incur a delay
		Given the node map
		 | a | b | c |

		And the nodes
		 | node | highway         |
		 | b    | traffic_signals |

		And the ways
		 | nodes |
		 | abc   |

		When I route I should get
		 | from | to | route | time    |
		 | a    | b  | abc   | 10s +-1 |
		 | b    | a  | abc   | 10s +-1 |

	Scenario: Routing between signals on the same way should not incur a delay
		Given the node map
		 | a | b | c | d |

		And the nodes
		 | node | highway         |
		 | a    | traffic_signals |
		 | d    | traffic_signals |

		And the ways
		 | nodes |
		 | abcd   |

		When I route I should get
		 | from | to | route | time    |
		 | b    | c  | abcd  | 10s +-1 |
		 | c    | b  | abcd  | 10s +-1 |

	Scenario: Prefer faster route without traffic signals
		Given the node map
		 | a | b | c |
		 |   | d |   |
	
		And the nodes
		 | node | highway         |
		 | b    | traffic_signals |

		And the ways
		 | nodes |
		 | abc   |
		 | adc   |
    
		When I route I should get
		 | from | to | route | 
		 | a    | c  | adc   |
