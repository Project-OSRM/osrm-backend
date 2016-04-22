@routing  @guidance
Feature: Merging

    Background:
        Given the profile "car"
        Given a grid size of 10 meters

    Scenario: Merge on Four Way Intersection
        Given the node map
            | d |   |   |
            | a | b | c |
            | e |   |   |

        And the ways
            | nodes | highway |
            | abc   | primary |
            | db    | primary |
            | eb    | primary |

       When I route I should get
            | waypoints | route      | turns                            |
            | d,c       | db,abc,abc | depart,merge slight right,arrive |
            | e,c       | eb,abc,abc | depart,merge slight left,arrive  |

    Scenario: Merge on Three Way Intersection Right
        Given the node map
            | d |   |   |
            | a | b | c |

        And the ways
            | nodes | highway |
            | abc   | primary |
            | db    | primary |

       When I route I should get
            | waypoints | route      | turns                            |
            | d,c       | db,abc,abc | depart,merge slight right,arrive |

    Scenario: Merge on Three Way Intersection Right
        Given the node map
            | a | b | c |
            | d |   |   |

        And the ways
            | nodes | highway |
            | abc   | primary |
            | db    | primary |

       When I route I should get
            | waypoints | route      | turns                           |
            | d,c       | db,abc,abc | depart,merge slight left,arrive |

