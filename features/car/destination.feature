@routing @car @destination
Feature: Destination only, no passing through

	Background:
		Given the speedprofile "car"
		
	Scenario: Destination only street
		Given the node map
		 | a |   |   |   |   |
		 |   | b | c | d |   |
		 |   |   |   |   | e |

		And the ways
		 | nodes | access      |
		 | ab    |             |
		 | bcd   | destination |
		 | de    |             |

		When I route I should get
		 | from | to | route     |
		 | a    | b  | ab        |
		 | a    | c  | ab,bcd    |
		 | a    | d  | ab,bcd    |
		 | a    | e  | ab,bcd,de |
		 | e    | d  | de        |
		 | e    | c  | de,bcd    |
		 | e    | b  | de,bcd    |
		 | e    | a  | de,bcd,ab |
		 | b    | c  | bcd       |
		 | b    | d  | bcd       |
		 | d    | c  | bcd       |
		 | d    | b  | bcd       |
		
	Scenario: Series of destination only streets
		Given the node map
		 | a |   | c |   | e |
		 |   | b |   | d |   |

		And the ways
		 | nodes | access      |
		 | ab    |             |
		 | bc    | destination |
		 | cd    | destination |
		 | de    | destination |

		When I route I should get
		 | from | to | route |
		 | a    | b  | ab    |
		 | a    | c  | ab    |
		 | a    | d  | ab    |
		 | a    | e  | ab    |

	Scenario: Routing inside a destination only area
		Given the node map
		 | a |   | c |   | e |
		 |   | b |   | d |   |

		And the ways
		 | nodes | access      |
		 | ab    | destination |
		 | bc    | destination |
		 | cd    | destination |
		 | de    | destination |

		When I route I should get
		 | from | to | route       |
		 | a    | e  | ab,bc,cd,de |
		 | e    | a  | de,cd,bc,ab |
		 | b    | d  | bc,cd       |
		 | d    | b  | cd,bc       |