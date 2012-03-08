@routing @ferry
Feature: Handle ferry routes
	
	Scenario: Use a ferry route
		Given the node map
		 | a | b | c |   |   |
		 |   |   | d |   |   |
		 |   |   | e | f | g |
	
		And the ways
		 | nodes | highway | route | bicycle |
		 | abc   | primary |       |         |
		 | cde   |         | ferry | yes     |
		 | efg   | primary |       |         |
   
		When I route I should get
		 | from | to | route       |
		 | a    | g  | abc,cde,efg |
		 | b    | f  | abc,cde,efg |
		 | e    | c  | cde         |
		 | e    | b  | cde,abc     |
		 | e    | a  | cde,abc     |
		 | c    | e  | cde         |
		 | c    | f  | cde,efg     |
		 | c    | g  | cde,efg     |


