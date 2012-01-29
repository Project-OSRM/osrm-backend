@outlines
Feature: Outlines
	Scenario outlines is another way to test routes... not sure which method is best?
	
	Scenario Outline: ways
		Given the nodes
		 | a |   | c |
		 |   | b |   |

		And the ways
		 | nodes |
		 | ab    |
		 | bc    |

	    When I route I between "<from>" and "<to>"
		Then I should get the route "<route>"
	
		Examples:
		 | from | to | route |
		 | a    | c  | abc   |
		 | c    | a  | cba   |
		 | a    | b  | ab    |
		 | b    | a  | ba    |
		 | b    | c  | bc    |
		 | c    | b  | cb    |
