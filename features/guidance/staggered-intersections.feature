@routing  @guidance @staggered-intersections
Feature: Staggered Intersections

    Background:
        Given the profile "car"
        Given a grid size of 1 meters
        # Note the one meter grid size: staggered intersections make zig-zags of a couple of meters only

    # https://www.openstreetmap.org/#map=19/39.26022/-84.25144
    Scenario: Staggered Intersection: Oak St, Cedar Dr
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
            | abc    | residential | Oak St   |
            | efg    | residential | Oak St   |
            | jcdehi | residential | Cedar Dr |

        When I route I should get
            | waypoints | route         | turns |
            | a,g       | Oak St,Oak St | depart,arrive |
            | g,a       | Oak St,Oak St | depart,arrive |

    Scenario: Staggered Intersection: do not collapse if long segment in between
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
            | abc    | residential | Oak St   |
            | efg    | residential | Oak St   |
            | jcdehi | residential | Cedar Dr |

        When I route I should get
            | waypoints | route                         | turns                              |
            | a,g       | Oak St,Cedar Dr,Oak St,Oak St | depart,turn right,turn left,arrive |
            | g,a       | Oak St,Cedar Dr,Oak St,Oak St | depart,turn right,turn left,arrive |

    Scenario: Staggered Intersection: do not collapse if not left-right or right-left
        Given the node map
            """
                j
            a b c
                d
            g f e
                h
                i
            """

        And the ways
            | nodes  | highway     | name     |
            | abc    | residential | Oak St   |
            | efg    | residential | Oak St   |
            | jcdehi | residential | Cedar Dr |

        When I route I should get
            | waypoints | route                | turns                        |
            | a,g       | Oak St,Oak St,Oak St | depart,continue uturn,arrive |
            | g,a       | Oak St,Oak St,Oak St | depart,continue uturn,arrive |

    Scenario: Staggered Intersection: do not collapse if the names are not the same
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
            | abc    | residential | Oak St   |
            | efg    | residential | Elm St   |
            | jcdehi | residential | Cedar Dr |

        When I route I should get
            | waypoints | route                         | turns                              |
            | a,g       | Oak St,Cedar Dr,Elm St,Elm St | depart,turn right,turn left,arrive |
            | g,a       | Elm St,Cedar Dr,Oak St,Oak St | depart,turn right,turn left,arrive |
