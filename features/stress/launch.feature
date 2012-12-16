@stress
Feature: Launching and shutting down
	
	Background:
		Given the profile "testbot"
	
	Scenario: Stress - 10km routing
	#osrm-routed hangs very often
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

 		When I route 100 times I should get
 		 | from | to | route |
 		 | x    | h  | xh    |

	Scenario: Stress - 10km routing
	#osrm-routed hangs sometimes
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

 		When I route 100 times I should get
 		 | from | to | route |
 		 | x    | a  | xa    |
 		 | x    | b  | xb    |
 		 | x    | c  | xc    |
 		 | x    | d  | xd    |
 		 | x    | e  | xe    |
 		 | x    | f  | xf    |
 		 | x    | g  | xg    |
 		 | x    | h  | xh    |
