@uturn @routing
Feature: Routing related to u-turns.

Scenario: Don't add u-turns when not needed.
	Given the node map
	 | a | b | c | d | e | f |

	And the ways
	 | nodes  |
	 | abcdef |

	When I route I should get
	 | from | to | route  |
	 | a    | b  | abcdef |
	 | a    | c  | abcdef |
	 | a    | d  | abcdef |
	 | a    | e  | abcdef |
	 | a    | f  | abcdef |
	 | b    | a  | abcdef |
	 | b    | c  | abcdef |
	 | b    | d  | abcdef |
	 | b    | e  | abcdef |
	 | b    | f  | abcdef |
	 | c    | a  | abcdef |
	 | c    | b  | abcdef |
	 | c    | d  | abcdef |
	 | c    | e  | abcdef |
	 | c    | f  | abcdef |
	 | d    | a  | abcdef |
	 | d    | b  | abcdef |
	 | d    | c  | abcdef |
	 | d    | e  | abcdef |
	 | d    | f  | abcdef |
	 | e    | a  | abcdef |
	 | e    | b  | abcdef |
	 | e    | c  | abcdef |
	 | e    | d  | abcdef |
	 | e    | f  | abcdef |
	 | f    | a  | abcdef |
	 | f    | b  | abcdef |
	 | f    | c  | abcdef |
	 | f    | d  | abcdef |
	 | f    | e  | abcdef |
