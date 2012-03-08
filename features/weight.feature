@routing @weight
Feature: Choosing route based on length, speed, etc
	
	Scenario: Pick the geometrically shortest route, way types being equal
		Given the node map
		 |   | s |   |
		 |   | t |   |
		 | a |   | b |

		And the ways
		 | nodes |
		 | atb   |
		 | asb   |

		When I route I should get
		 | from | to | route |
		 | a    | b  | atb   |
		 | a    | b  | atb   |

	Scenario: Pick the fastest way type, lengths being equal
		Given the node map
		 | a | s |
		 | p | b |
	
		And the ways
		 | nodes | highway   |
		 | apb   | primary   |
		 | asb   | secondary |

