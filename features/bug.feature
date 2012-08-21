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
#note that this is very similar to the example above, exept that the node map is mirrored
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
