@uturn @routing
Feature: Routing related to u-turns.

Scenario: Don't add u-turns when not needed.
	Given the node map
	 | a | b | c | d | e | f |

	And the ways
	 | nodes  |
	 | abcdef |

	When I route I should get
	 | from | to | route  | compass | bearing |
	 | a    | b  | abcdef | E       | 90      |
	 | a    | c  | abcdef | E       | 90      |
	 | a    | d  | abcdef | E       | 90      |
	 | a    | e  | abcdef | E       | 90      |
	 | a    | f  | abcdef | E       | 90      |
	 | b    | a  | abcdef | W       | 270     |
	 | b    | c  | abcdef | E       | 90      |
	 | b    | d  | abcdef | E       | 90      |
	 | b    | e  | abcdef | E       | 90      |
	 | b    | f  | abcdef | E       | 90      |
	 | c    | a  | abcdef | W       | 90      |
	 | c    | b  | abcdef | W       | 270     |
	 | c    | d  | abcdef | E       | 90      |
	 | c    | e  | abcdef | E       | 90      |
	 | c    | f  | abcdef | E       | 90      |
	 | d    | a  | abcdef | W       | 270     |
	 | d    | b  | abcdef | W       | 270     |
	 | d    | c  | abcdef | W       | 270     |
	 | d    | e  | abcdef | E       | 90      |
	 | d    | f  | abcdef | E       | 90      |
	 | e    | a  | abcdef | W       | 270     |
	 | e    | b  | abcdef | W       | 270     |
	 | e    | c  | abcdef | W       | 270     |
	 | e    | d  | abcdef | W       | 270     |
	 | e    | f  | abcdef | E       | 90      |
	 | f    | a  | abcdef | W       | 270     |
	 | f    | b  | abcdef | W       | 270     |
	 | f    | c  | abcdef | W       | 270     |
	 | f    | d  | abcdef | W       | 270     |
	 | f    | e  | abcdef | W       | 270     |
