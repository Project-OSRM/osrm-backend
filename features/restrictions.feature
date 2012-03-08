@routing @restrictions
Feature: Turn restrictions
	Handle turn restrictions as defined by http://wiki.openstreetmap.org/wiki/Relation:restriction
	Note that if u-turns are allowed, turn restrictions can lead to suprising, but correct, routes.
	
	Background: Use car routing
		Given the speedprofile "car"
	
	@no_turning
	Scenario: No left turn
		Given the node map
		 |   | n |   |
		 | w | j | e |
		 |   | s |   |

		And the ways
		 | nodes | oneway  |
		 | sj    | yes     |
		 | nj    | -1      |
		 | wj    | -1      |
		 | ej    | -1      |

		And the relations
		 | from | to | via | restriction  |
		 | sj   | wj | j   | no_left_turn |

		When I route I should get
		 | from | to | route |
		 | s    | w  |       |
		 | s    | n  | sj,nj |
		 | s    | e  | sj,ej |

	@no_turning
	Scenario: No right turn
		Given the node map
		 |   | n |   |
		 | w | j | e |
		 |   | s |   |

		And the ways
		 | nodes | oneway  |
		 | sj    | yes     |
		 | nj    | -1      |
		 | wj    | -1      |
		 | ej    | -1      |

		And the relations
		 | from | to | via | restriction   |
		 | sj   | ej | j   | no_right_turn |

		When I route I should get
		 | from | to | route |
		 | s    | w  | sj,wj |
		 | s    | n  | sj,nj |
		 | s    | e  |       |

	@no_turning
	Scenario: No u-turn
		Given the node map
		 |   | n |   |
		 | w | j | e |
		 |   | s |   |

		And the ways
		 | nodes | oneway  |
		 | sj    | yes     |
		 | nj    | -1      |
		 | wj    | -1      |
		 | ej    | -1      |

		And the relations
		 | from | to | via | restriction |
		 | sj   | wj | j   | no_u_turn   |

		When I route I should get
		 | from | to | route |
		 | s    | w  |       |
		 | s    | n  | sj,nj |
		 | s    | e  | sj,ej |

	@no_turning
	Scenario: Handle any no_* relation
		Given the node map
		 |   | n |   |
		 | w | j | e |
		 |   | s |   |

		And the ways
		 | nodes | oneway  |
		 | sj    | yes     |
		 | nj    | -1      |
		 | wj    | -1      |
		 | ej    | -1      |

		And the relations
		 | from | to | via | restriction      |
		 | sj   | wj | j   | no_weird_zigzags |

		When I route I should get
		 | from | to | route |
		 | s    | w  |       |
		 | s    | n  | sj,nj |
		 | s    | e  | sj,ej |

	@only_turning
	Scenario: Only left turn
		Given the node map
		 |   | n |   |
		 | w | j | e |
		 |   | s |   |

		And the ways
		 | nodes | oneway  |
		 | sj    | yes     |
		 | nj    | -1      |
		 | wj    | -1      |
		 | ej    | -1      |

		And the relations
		 | from | to | via | restriction    |
		 | sj   | wj | j   | only_left_turn |

		When I route I should get
		 | from | to | route |
		 | s | w | sj,wj |
		 | s | n |       |
		 | s | e |       |

	@only_turning
	Scenario: Only right turn
		Given the node map
		 |   | n |   |
		 | w | j | e |
		 |   | s |   |

		And the ways
		 | nodes | oneway  |
		 | sj    | yes     |
		 | nj    | -1      |
		 | wj    | -1      |
		 | ej    | -1      |

		And the relations
		 | from | to | via | restriction     |
		 | sj   | ej | j   | only_right_turn |

		When I route I should get
		 | from | to | route |
		 | s    | w  |       |
		 | s    | n  |       |
		 | s    | e  | sj,ej |
	
	@only_turning
	Scenario: Only straight on
		Given the node map
		 |   | n |   |
		 | w | j | e |
		 |   | s |   |

		And the ways
		 | nodes | oneway  |
		 | sj    | yes     |
		 | nj    | -1      |
		 | wj    | -1      |
		 | ej    | -1      |

		And the relations
		 | from | to | via | restriction      |
		 | sj   | nj | j   | only_straight_on |

		When I route I should get
		 | from | to | route |
		 | s    | w  |       |
		 | s    | n  | sj,nj |
		 | s    | e  |       |

	@no_turning
	Scenario: Handle any only_* restriction
		Given the node map
		 |   | n |   |
		 | w | j | e |
		 |   | s |   |

		And the ways
		 | nodes | oneway  |
		 | sj    | yes     |
		 | nj    | -1      |
		 | wj    | -1      |
		 | ej    | -1      |

		And the relations
		 | from | to | via | restriction        |
		 | sj   | nj | j   | only_weird_zigzags |

		When I route I should get
		 | from | to | route |
		 | s    | w  |       |
		 | s    | n  | sj,nj |
		 | s    | e  |       |
