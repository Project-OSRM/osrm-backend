@routing @penalty
Feature: Penalties
	
	Background:
		Given the speedprofile "bicycle"
		And the speedprofile settings
		 | trafficSignalPenalty | 20 |
			
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
		 | from | to | route   | time |
		 | a    | c  | abc     | 38s  |
		 | d    | f  | def     | 58s  |

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
		 | from | to | route | time |
		 | a    | e  | abcde | 136s |

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
		 | from | to | route   | time |
		 | a    | b  | abc     | 19s  |
		 | b    | a  | abc     | 19s  |
		 | b    | c  | abc     | 19s  |
		 | c    | b  | abc     | 19s  |

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
		 | from | to | route   | time |
		 | b    | c  | abcd    | 19s  |
		 | c    | b  | abcd    | 19s  |

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
		 | a    | c | adc   |
