@routing  @guidance @staggered-intersections
Feature: Staggered Intersections

    Background:
        Given the profile "bicycle"
        Given a grid size of 1 meters
        # Note the one meter grid size: staggered intersections make zig-zags of a couple of meters only

    Scenario: Staggered Intersection - pushing in the middle
        Given the node map
            """
                j
            a b c
                d
                e f g
                h
                i
            """

        And the ways
            | nodes  | highway     | name     | oneway |
            | abc    | residential | Oak St   |        |
            | efg    | residential | Oak St   |        |
            | ihedcj | residential | Cedar Dr | yes    |

        When I route I should get
            | waypoints | route                         | turns                              | modes                                |
            | a,g       | Oak St,Cedar Dr,Oak St,Oak St | depart,turn right,turn left,arrive | cycling,pushing bike,cycling,cycling |
            | g,a       | Oak St,Oak St                 | depart,arrive                      | cycling,cycling                      |

    Scenario: Staggered Intersection - pushing at start
        Given the node map
            """
                j
            a b c
                d
                e f g
                h
                i
            """

        And the ways
            | nodes  | highway     | name     | oneway |
            | cba    | residential | Oak St   | yes    |
            | efg    | residential | Oak St   |        |
            | ihedcj | residential | Cedar Dr |        |

        When I route I should get
            | waypoints | route                         | turns                              | modes                                |
            | a,g       | Oak St,Cedar Dr,Oak St,Oak St | depart,turn right,turn left,arrive | pushing bike,cycling,cycling,cycling |
            | g,a       | Oak St,Oak St                 | depart,arrive                      | cycling,cycling                      |

    Scenario: Staggered Intersection - pushing at end
        Given the node map
            """
                j
            a b c
                d
                e f g
                h
                i
            """

        And the ways
            | nodes  | highway     | name     | oneway |
            | abc    | residential | Oak St   |        |
            | gfe    | residential | Oak St   | yes    |
            | ihedcj | residential | Cedar Dr |        |

        When I route I should get
            | waypoints | route         | turns                                              | modes                                     |
            | a,g       | Oak St,Cedar Dr,Oak St,Oak St | depart,turn right,turn left,arrive | cycling,cycling,pushing bike,pushing bike |
            | g,a       | Oak St,Oak St | depart,arrive                                      | cycling,cycling                           |

    Scenario: Staggered Intersection - pushing at start and end
        Given the node map
            """
                j
            a b c
                d
                e f g
                h
                i
            """

        And the ways
            | nodes  | highway     | name     | oneway |
            | cba    | residential | Oak St   | yes    |
            | gfe    | residential | Oak St   | yes    |
            | ihedcj | residential | Cedar Dr |        |

        When I route I should get
            | waypoints | route                         | turns                              | modes                                          |
            | a,g       | Oak St,Cedar Dr,Oak St,Oak St | depart,turn right,turn left,arrive | pushing bike,cycling,pushing bike,pushing bike |
            | g,a       | Oak St,Oak St                 | depart,arrive                      | cycling,cycling                                |

    Scenario: Staggered Intersection - pushing at start and end
        Given the node map
            """
                j
            a b c
                d
                e f g
                h
                i
            """

        And the ways
            | nodes  | highway     | name     |
            | cba    | pedestrian  | Oak St   |
            | gfe    | pedestrian  | Oak St   |
            | ihedcj | residential | Cedar Dr |

        When I route I should get
            | waypoints | route                         | turns                              | modes                                          |
            | a,g       | Oak St,Cedar Dr,Oak St,Oak St | depart,turn right,turn left,arrive | pushing bike,cycling,pushing bike,pushing bike |
            | g,a       | Oak St,Cedar Dr,Oak St,Oak St | depart,turn right,turn left,arrive | pushing bike,cycling,pushing bike,pushing bike |

    Scenario: Staggered Intersection - control, all cycling on staggered intersection
        Given the node map
            """
                j
            a b c
                d
                e f g
                h
                i
            """

        And the ways
            | nodes  | highway     | name     |
            | cba    | residential | Oak St   |
            | gfe    | residential | Oak St   |
            | ihedcj | residential | Cedar Dr |

        When I route I should get
            | waypoints | route         | turns         | modes           |
            | a,g       | Oak St,Oak St | depart,arrive | cycling,cycling |
            | g,a       | Oak St,Oak St | depart,arrive | cycling,cycling |
