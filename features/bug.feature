@bug
Feature: Things that causes crashes or hangs

	@nohang
	Scenario: OK
	#this works as expected
		Given the node map
		 | a |   |
		 | b | c |
	
		And the ways
		 | nodes |
		 | ab    |
		 | cb    |

		When I route I should get
		 | from | to | route |
		 | c    | b  | cb    |

	@hang
	Scenario: Routed hangs on simple ways
	#this causes osrm-routed to hang (at least on mac 10.8)
	#note that this is very similar to the example above, except that the node map is mirrored
		Given the node map
		|   | a |
		| c | b |
	
		And the ways
		 | nodes |
		 | ab    |
		 | cb    |

		When I route I should get
		 | from | to | route |
		 | c    | b  | cb    |

	@crash
	Scenario: Quarter way around the equator
		Given the node locations
		 | node | lat | lon |
		 | a    | 0   | 0   |
		 | b    | 0   | 90  |

		And the ways
		 | nodes |
		 | ab    |

		When I route I should get
		 | from | to | route |
		 | a    | b  | ab    |

	@crash
	Scenario: From the equator to the north pole
		Given the node locations
		 | node | lat | lon |
		 | a    | 0   | 0   |
		 | b    | 90  | 0   |

		And the ways
		 | nodes |
		 | ab    |

		When I route I should get
		 | from | to | route |
		 | a    | b  | ab    |
