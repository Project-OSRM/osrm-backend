@routing @bad
Feature: Handle bad data in a graceful manner
	
	Scenario: Empty dataset
		Given the node map
		 | a | b |

		Given the ways
		 | nodes |
		
		When I route I should get
		 | from | to | route |
		 | a    | b  |       |

	Scenario: Start/end point at the same location
		Given the node map
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
		Given the node map
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

