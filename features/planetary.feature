@routing @planetary
Feature: Distance calculation
#reference distances have been calculated usign http://seismo.cqu.edu.au/CQSRG/VDistance/
 
	Scenario: Longitudinal distances at equator
		Given the node locations
		 | node | lat | lon |
		 | a    | 0   | 80  |
		 | b    | 0   | 0   |

		And the ways
		 | nodes |
		 | ab    |

		When I route I should get
		 | from | to | route | distance      |
		 | a    | b  | ab    | 8905559 ~0.1% |

	Scenario: Longitudinal distances at latitude 45
		Given the node locations
		 | node | lat | lon |
		 | c    | 45   | 80 |
		 | d    | 45   | 0  |

		And the ways
		 | nodes |
		 | cd    |

		When I route I should get
		 | from | to | route | distance      |
		 | c    | d  | cd    | 6028844 ~0.5% |

	Scenario: Longitudinal distances at latitude 80
		Given the node locations
		 | node | lat | lon |
		 | c    | 80   | 80 |
		 | d    | 80   | 0  |

		And the ways
		 | nodes |
		 | cd    |

		When I route I should get
		 | from | to | route | distance      |
		 | c    | d  | cd    | 1431469 ~0.5% |

	Scenario: Latitudinal distances at longitude 0
		Given the node locations
		 | node | lat | lon |
		 | a    | 80  | 0   |
		 | b    | 0   | 0   |

		And the ways
		 | nodes |
		 | ab    |

		When I route I should get
		 | from | to | route | distance      |
		 | a    | b  | ab    | 8905559 ~0.1% |

	Scenario: Latitudinal distances at longitude 45
		Given the node locations
		 | node | lat | lon |
		 | a    | 80  | 45   |
		 | b    | 0   | 45   |

		And the ways
		 | nodes |
		 | ab    |

		When I route I should get
		 | from | to | route | distance      |
		 | a    | b  | ab    | 8905559 ~0.1% |

	Scenario: Latitudinal distances at longitude 80
		Given the node locations
		 | node | lat | lon |
		 | a    | 80  | 80   |
		 | b    | 0   | 80   |

		And the ways
		 | nodes |
		 | ab    |

		When I route I should get
		 | from | to | route | distance      |
		 | a    | b  | ab    | 8905559 ~0.1% |		 