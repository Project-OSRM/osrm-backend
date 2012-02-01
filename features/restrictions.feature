@routing @restrictions
Feature: Turn restrictions
	Handle turn restrictions as defined by http://wiki.openstreetmap.org/wiki/Relation:restriction

	Scenario Outline: No left turn at T-junction
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
	    
		When I route between "<from>" and "<to>"
		Then "<route>" should be returned
		
		Examples:
		 | from | to | route |
		 | a    | b  | ajb   |
		 | a    | s  | ajs   |
		 | b    | a  | bja   |
		 | b    | s  | bjs   |
		 | s    | a  |       |
		 | s    | b  | sjb   |