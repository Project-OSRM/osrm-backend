@routing @basic
Feature: Basic Routing

	Scenario Outline: Smallest possible datasat
 		Given the nodes
		 | a | b |
		
		And the ways
		 | nodes |
		 | ab    |
	    
		When I route between "<from>" and "<to>"
		Then "<route>" should be returned
		
		Examples:
		 | from | to | route |
		 | a    | b  | ab    |
		 | b    | a  | ba    |
		
	Scenario Outline: Smallest possible datasat
		Given the nodes
		 | a | b |
		
		And the ways
		 | nodes |
		 | ab    |
	    
		When I route between "<from>" and "<to>"
		Then "<route>" should be returned
		
		Examples:
		 | from | to | route |
		 | a    | b  | ab    |
		 | b    | a  | ba    |

	Scenario Outline: Connected ways
		Given the nodes
		 | a |   | c |
		 |   | b |   |		
		
		And the ways
		 | nodes |
		 | ab    |
		 | bc    |
	    
		When I route between "<from>" and "<to>"
		Then "<route>" should be returned
		
		Examples:
		 | from | to | route |
		 | a    | c  | abc   |
		 | c    | a  | cba   |
		 | a    | b  | ab    |
		 | b    | a  | ba    |
		 | b    | c  | bc    |
		 | c    | b  | cb    |

	Scenario Outline: Unconnected ways
		Given the nodes
		 | a | b |
		 | c | d |
		
		And the ways
		 | nodes |
		 | ab    |
		 | cd    |
	    
		When I route between "<from>" and "<to>"
		Then "<route>" should be returned
		
		Examples:
		 | from | to | route |
		 | a    | b  | ab    |
		 | b    | a  | ba    |
		 | c    | d  | cd    |
		 | d    | c  | dc    |
		 | a    | c  |       |
		 | c    | a  |       |
		 | b    | d  |       |
		 | d    | c  |       |
		 | a    | d  |       |
		 | d    | a  |       |

	Scenario Outline: Pick the fastest way type
		Given the nodes
		 | a | s |
		 | p | b |
		
		And the ways
		 | nodes | highway   |
		 | apb   | primary   |
		 | asb   | secondary |
	    
		When I route between "<from>" and "<to>"
		Then "<route>" should be returned
		
		Examples:
		 | from | to | route |
		 | a    | b  | apb   |
		 | b    | a  | bpa   |