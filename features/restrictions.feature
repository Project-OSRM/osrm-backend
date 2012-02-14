@routing @restrictions
Feature: Turn restrictions
	Handle turn restrictions as defined by http://wiki.openstreetmap.org/wiki/Relation:restriction
	How this plays with u-turns can be tricky.
	
	Scenario: No left turn
		Given the nodes
		 |   | t |   |
		 | a | j | b |
		 |   | s |   |

		And the ways
		 | nodes |
		 | bj    |
		 | aj    |
		 | sj    |
		 | tj    |

		And the relations
		 | from | to | via | restriction  |
		 | sj   | aj | j   | no_left_turn |

		When I route I should get
		 | from | to | route |
		 | s    | a  |       |
		 | s    | b  | sj,jb |
		 | s    | t  | sj,tj |
		 | a    | b  | aj,bj |
		 | a    | a  | aj,sj |
		 | a    | t  | aj,tj |
		 | b    | b  | jb,aj |
		 | b    | s  | bj,sj |
		 | b    | t  | bj,tj |

	Scenario: No left turn, go counter-clockwise around the block instead
		Given the nodes
		 | x | t |   |
		 | a | j | b |
		 |   | s |   |

		And the ways
		 | nodes |
		 | bj    |
		 | aj    |
		 | sj    |
		 | tj    |
		 | axt   |

		And the relations
		 | from | to | via | restriction  |
		 | sj   | aj | j   | no_left_turn |

		When I route I should get
		 | from | to | route     |
		 | s    | a  | sj,tj,axt |
		 | s    | b  | sj,jb     |
		 | s    | t  | sj,tj     |
		 | a    | b  | aj,bj     |
		 | a    | a  | aj,sj     |
		 | a    | t  | aj,tj     |
		 | b    | b  | jb,aj     |
		 | b    | s  | bj,sj     |
		 | b    | t  | bj,tj     |
		
	Scenario: No left turn, go clockwise around the block instead
		Given the nodes
		 |   |   | t |   |
		 | z | a | j | b |
		 | x |   | s |   |

		And the ways
		 | nodes |
		 | bj    |
		 | aj    |
		 | sj    |
		 | tj    |
		 | sxza  |

		And the relations
		 | from | to | via | restriction  |
		 | sj   | aj | j   | no_left_turn |

		When I route I should get
		 | from | to | route     |
		 | s    | a  | sxza      |
		 | s    | b  | sj,jb     |
		 | s    | t  | sj,tj     |
		 | a    | b  | aj,bj     |
		 | a    | a  | aj,sj     |
		 | a    | t  | aj,tj     |
		 | b    | b  | jb,aj     |
		 | b    | s  | bj,sj     |
		 | b    | t  | bj,tj     |


