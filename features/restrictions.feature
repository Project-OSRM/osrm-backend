@routing @restrictions
Feature: Turn restrictions
	OSRM should handle turn restrictions as defined by http://wiki.openstreetmap.org/wiki/Relation:restriction

	Scenario: No left turn at T-junction
		Given the nodes
		 | a | j | b |
		 |   | s |   |

		And the ways
		 | nodes |
		 | aj    |
		 | jb    |
		 | sj    |

		And the relations
		 | from | to | via | restriction  |
		 | sj   | ja | j   | no_left_turn |

	    When I route I should get
		 | from | to | route |
		 | a    | b  | ajb   |
		 | a    | s  | ajs   |
		 | b    | a  | bja   |
		 | b    | s  | bjs   |
		 | s    | a  |       |
		 | s    | b  | sjb   |

