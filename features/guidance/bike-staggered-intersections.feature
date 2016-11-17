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
            | waypoints | route         | turns         |
            | a,g       | Oak St,Oak St | depart,arrive |
            | g,a       | Oak St,Oak St | depart,arrive |

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
            | waypoints | route         | turns         |
            | a,g       | Oak St,Oak St | depart,arrive |
            | g,a       | Oak St,Oak St | depart,arrive |

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
            | waypoints | route         | turns         |
            | a,g       | Oak St,Oak St | depart,arrive |
            | g,a       | Oak St,Oak St | depart,arrive |

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
            | waypoints | route         | turns         |
            | a,g       | Oak St,Oak St | depart,arrive |
            | g,a       | Oak St,Oak St | depart,arrive |
