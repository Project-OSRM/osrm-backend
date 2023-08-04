@routing @disable-feature-dataset
Feature: disable-feature-dataset command line options
    Background:
        Given the profile "testbot"
        And the node map
            """
            0
            a b c
            """
        And the ways
            | nodes |
            | ab    |
            | bc    |

    Scenario: disable-feature-dataset - geometry disabled error
        Given the data load extra arguments "--disable-feature-dataset ROUTE_GEOMETRY"

        # The default values
        And the query options
          | overview       | simplified |
          | annotations    | false      |
          | steps          | false      |
          | skip_waypoints | false      |

        When I route I should get
            | from | to | code              |
            | a    | c  | DisabledDataset  |

        When I plan a trip I should get
            | waypoints | code             |
            | a,b,c     | DisabledDataset |

        When I match I should get
          | trace | code             |
          | abc   | DisabledDataset |

    Scenario: disable-feature-dataset - geometry disabled error table
        Given the data load extra arguments "--disable-feature-dataset ROUTE_GEOMETRY"

        When I request nearest I should get
            | in | code              |
            | 0  | DisabledDataset  |

        When I request a travel time matrix with these waypoints I should get the response code
            | waypoints  | code             |
            | a,b,c      | DisabledDataset |


    Scenario: disable-feature-dataset - geometry disabled success
        Given the data load extra arguments "--disable-feature-dataset ROUTE_GEOMETRY"

        # No geometry values returned
        And the query options
          | overview       | false |
          | annotations    | false |
          | steps          | false |
          | skip_waypoints | true  |

        When I route I should get
            | from | to | code     |
            | a    | c  | Ok       |

        When I plan a trip I should get
            | waypoints |  code |
            | a,b,c     |  Ok   |

        When I match I should get
          | trace | code |
          | abc   | Ok   |

    Scenario: disable-feature-dataset - geometry disabled error table
        Given the data load extra arguments "--disable-feature-dataset ROUTE_GEOMETRY"

        And the query options
          | skip_waypoints | true  |

        # You would never do this, but just to prove the point.
        When I request nearest I should get
            | in | code              |
            | 0  | Ok                |

        When I request a travel time matrix with these waypoints I should get the response code
            | waypoints  | code  |
            | a,b,c      | Ok    |


    Scenario: disable-feature-dataset - steps disabled error
        Given the data load extra arguments "--disable-feature-dataset ROUTE_STEPS"

        # Default + annotations, steps
	And the query options
          | overview       | simplified |
          | annotations    | true       |
          | steps          | true       |

        When I route I should get
            | from | to | code               |
            | a    | c  | DisabledDataset   |

        When I plan a trip I should get
            | waypoints |  code            |
            | a,b,c     | DisabledDataset |

        When I match I should get
          | trace | code             |
          | abc   | DisabledDataset |


    Scenario: disable-feature-dataset - geometry disabled error table
        Given the data load extra arguments "--disable-feature-dataset ROUTE_STEPS"

        When I request nearest I should get
            | in | code |
            | 0  | Ok   |

        When I request a travel time matrix with these waypoints I should get the response code
            | waypoints  | code  |
            | a,b,c      | Ok    |


    Scenario: disable-feature-dataset - steps disabled success
        Given the data load extra arguments "--disable-feature-dataset ROUTE_STEPS"

        # Default + steps
        And the query options
          | overview       | simplified |
          | annotations    | true       |
          | steps          | false      |

        When I route I should get
            | from | to | code     |
            | a    | c  | Ok       |

        When I plan a trip I should get
            | waypoints |  code |
            | a,b,c     |  Ok   |

        When I match I should get
          | trace | code |
          | abc   | Ok   |

