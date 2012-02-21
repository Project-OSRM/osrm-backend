@routing @bad
Feature: Handle bad data in a graceful manner
	
	Scenario: Empty dataset
		Given the nodes
		 | a | b |

		Given the ways
		 | nodes |
		
		When I route I should get
		 | from | to | route |
		 | a    | b  |       |

	Scenario: Start/end point at the same location
		Given the nodes
		 | a | b |
		 | 1 | 2 |

		Given the ways
		 | nodes |
		 | ab    |

		When I route I should get
		 | from | to | route |
		 | a    | a  |       |
		 | b    | b  |       |
		 | 1    | 1  |       |
		 | 2    | 2  |       |

	Scenario: Start/end point far outside data area
		Given the nodes
		 |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |    | 1 |
		 | a | b |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |    | 2 |
		 |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |    | 3 |

		Given the ways
		 | nodes |
		 | ab    |

		When I route I should get
		 | from | to | route |
		 | 1    | a  | ab    |
		 | 2    | a  | ab    |
		 | 3    | a  | ab    |
		 | 1    | b  |       |
		 | 2    | b  |       |
		 | 3    | b  |       |
		 | 1    | 2  |       |
		 | 1    | 3  |       |
		 | 2    | 1  |       |
		 | 2    | 3  |       |
		 | 3    | 1  |       |
		 | 3    | 2  |       |

	@bad_oneway
	Scenario: Invalid oneway dead-ends
		Given the nodes
		 | a | x | b |

		And the ways
		 | nodes | oneway |
		 | ax    | -1     |
		 | xb    | yes    |

		When I route I should get
		 | from | to | route |
		 | x    | a  | ax    |
		 | x    | b  | xb    |

	@bad_oneway
	Scenario: Impossible-to-leave oneway dead-ends
		Given the nodes
		 |   | t |   |
		 | a | x | b |
		 |   | s |   |

		And the ways
		 | nodes | oneway |
		 | ax    | -1     |
		 | xb    | yes    |
		 | sxt   |        |

		When I route I should get
		 | from | to | route |
		 | x    | a  | ax    |
		 | x    | b  | xb    |
		 | a    | x  |       |
		 | b    | x  |       |
	
	@bad_oneway
	Scenario: Impossible-to-reach oneway dead-ends
		Given the nodes
		 |   | t |   |
		 | a | x | b |
		 |   | s |   |

		And the ways
		 | nodes | oneway |
		 | ax    | yes    |
		 | xb    | -1     |
		 | sxt   |        |

		When I route I should get
		 | from | to | route |
		 | a    | x  | ax    |
		 | b    | b  | xb    |
		 | x    | a  |       |
		 | x    | b  |       |

	@bad_oneway
	Scenario: Impossible-to-leave oneway junctions
		Given the nodes
		 | t |   |   |
		 | a | x | b |
		 | s |   |   |

		And the ways
		 | nodes | oneway |
		 | ax    | yes    |
		 | bx    | yes    |
		 | sat   |        |

		When I route I should get
		 | from | to | route |
		 | a    | x  | ax    |
		 | b    | x  | xb    |
		 | x    | a  |       |
		 | x    | b  |       |
