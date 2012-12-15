@stress @launch
Feature: Launching and shutting down
	
	Background:
		Given the speedprofile "testbot"
	
	Scenario: Repeated launch and shutdown
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