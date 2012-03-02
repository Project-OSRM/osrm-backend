@routing @restrictions
Feature: Turn restrictions
	Handle turn restrictions as defined by http://wiki.openstreetmap.org/wiki/Relation:restriction
	Note that if u-turns are allowed, turn restrictions can lead to suprising, but correct, routes.

	#left side	
	@restriction_left @no_left_turn @no_turn
	Scenario: No left turn, no way to reach destination
		Given the nodes
		 | w | j |
		 |   | s |

		And the ways
		 | nodes |
		 | sj    |
		 | wj    |

		And the relations
		 | from | to | via | restriction  |
		 | sj   | wj | j   | no_left_turn |

		When I route I should get
		 | from | to | route |
		 | s    | w  |       |
		 | w    | s  | wj,sj |

	@restriction_left  @no_left_turn @no_turn
	Scenario: No left turn, don't use u-turns to reach destination
		Given the nodes
		 |   | n |   |
		 | w | j | e |
		 |   | s |   |

		And the ways
		 | nodes |
		 | nj    |
		 | ej    |
		 | sj    |
		 | wj    |

		And the relations
		 | from | to | via | restriction  |
		 | sj   | wj | j   | no_left_turn |

		When I route I should get
		 | from | to | route |
		 | s    | w  |       |
		 | s    | n  | sj,nj |
		 | s    | e  | sj,ej |
		 | w    | n  | wj,nj |
		 | w    | e  | wj,ej |
		 | w    | s  | wj,sj |
		 | n    | e  | nj,ej |
		 | n    | s  | nj,sj |
		 | n    | w  | nj,wj |
		 | e    | s  | ej,sj |
		 | e    | w  | ej,wj |
		 | e    | n  | ej,nj |

	@restriction_left @no_left_turn @no_turn
	Scenario: No left turn, go around the block instead
		Given the nodes
		 | y | n |   |
		 | w | j | e |
		 |   | s |   |

		And the ways
		 | nodes |
		 | nj    |
		 | ej    |
		 | sj    |
		 | wj    |
		 | wyn   |

		And the relations
		 | from | to | via | restriction  |
		 | sj   | wj | j   | no_left_turn |

		When I route I should get
		 | from | to | route     |
		 | s    | w  | sj,nj,wyn |
		 | s    | n  | sj,nj     |
		 | s    | e  | sj,ej     |
		 | w    | n  | wj,nj     |
		 | w    | e  | wj,ej     |
		 | w    | s  | wj,sj     |
		 | n    | e  | nj,ej     |
		 | n    | s  | nj,sj     |
		 | n    | w  | nj,wj     |
		 | e    | s  | ej,sj     |
		 | e    | w  | ej,wj     |
		 | e    | n  | ej,nj     |
	
	@restriction_left @only_left_turn @must_turn
	Scenario: Only left turn, don't use u-turns to reach destination
		Given the nodes
		 |   | n |   |
		 | w | j | e |
		 |   | s |   |

		And the ways
		 | nodes |
		 | nj    |
		 | ej    |
		 | sj    |
		 | wj    |

		And the relations
		 | from | to | via | restriction    |
		 | sj   | wj | j   | only_restriction_left |

		When I route I should get
		 | from | to | route |
		 | s | w | sj,wj |
		 | s | n |       |
		 | s | e |       |
		 | w | n | wj,nj |
		 | w | e | wj,ej |
		 | w | s | wj,sj |
		 | n | e | nj,ej |
		 | n | s | nj,sj |
		 | n | w | nj,wj |
		 | e | s | ej,sj |
		 | e | w | ej,wj |
		 | e | n | ej,nj |

	#right side
	@restriction_right @no_right_turn @no_turn
	Scenario: No right turn, no way to reach destination
		Given the nodes
		 | j | e |
		 | s |   |

		And the ways
		 | nodes |
		 | ej    |
		 | sj    |
		 
		And the relations
		 | from | to | via | restriction   |
		 | sj   | ej | j   | no_right_turn |

		When I route I should get
		 | from | to | route |
		 | s    | e  |       |
		 | e    | s  | ej,sj |
		 
	@restriction_right @no_right_turn @no_turn
	Scenario: No right turn, don't use u-turns to reach destination
		Given the nodes
		 |   | n |   |
		 | w | j | e |
		 |   | s |   |

		And the ways
		 | nodes |
		 | nj    |
		 | ej    |
		 | sj    |
		 | wj    |

		And the relations
		 | from | to | via | restriction   |
		 | sj   | ej | j   | no_right_turn |

		When I route I should get
		 | from | to | route |
		 | s    | w  | sj,wj |
		 | s    | n  | sj,nj |
		 | s    | e  |       |
		 | w    | n  | wj,nj |
		 | w    | e  | wj,ej |
		 | w    | s  | wj,sj |
		 | n    | e  | nj,ej |
		 | n    | s  | nj,sj |
		 | n    | w  | nj,wj |
		 | e    | s  | ej,sj |
		 | e    | w  | ej,wj |
		 | e    | n  | ej,nj |

	@restriction_right @no_right_turn @no_turn
	Scenario: No right turn, go around the block instead
		Given the nodes
		 |   | n | y |
		 | w | j | e |
		 |   | s |   |

		And the ways
		 | nodes |
		 | nj    |
		 | ej    |
		 | sj    |
		 | wj    |
		 | nye   |

		And the relations
		 | from | to | via | restriction  |
		 | sj   | ej | j   | no_right_turn |

		When I route I should get
		 | from | to | route     |
		 | s    | w  | sj,wj     |
		 | s    | n  | sj,nj     |
		 | s    | e  | sj,nj,nye |
		 | w    | n  | wj,nj     |
		 | w    | e  | wj,ej     |
		 | w    | s  | wj,sj     |
		 | n    | e  | nj,ej     |
		 | n    | s  | nj,sj     |
		 | n    | w  | nj,wj     |
		 | e    | s  | ej,sj     |
		 | e    | w  | ej,wj     |
		 | e    | n  | ej,nj     |

	@restriction_right @only_right_turn @must_turn
	Scenario: Right turn only, don't use u-turns to reach destination
		Given the nodes
		 |   | n |   |
		 | w | j | e |
		 |   | s |   |

		And the ways
		 | nodes |
		 | nj    |
		 | ej    |
		 | sj    |
		 | wj    |

		And the relations
		 | from | to | via | restriction   |
		 | sj   | ej | j   | only_right_turn |

		When I route I should get
		 | from | to | route |
		 | s    | w  |       |
		 | s    | n  |       |
		 | s    | e  | sj,ej |
		 | w    | n  | wj,nj |
		 | w    | e  | wj,ej |
		 | w    | s  | wj,sj |
		 | n    | e  | nj,ej |
		 | n    | s  | nj,sj |
		 | n    | w  | nj,wj |
		 | e    | s  | ej,sj |
		 | e    | w  | ej,wj |
		 | e    | n  | ej,nj |

	@no_u_turn
	Scenario: No U-turn
		Given the nodes
		 |   | b |   |
		 |   |   |   |
		 | c |   | a |

		And the ways
		 | nodes |
		 | ab    |
		 | bc    |

		And the relations
		 | from | to | via | restriction |
		 | ab   | bc | b   | no_u_turn   |

		When I route I should get
		 | from | to | route |
		 | a    | c  |       |
		 | c    | a  | bc,ab |
