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
            | waypoints | route         | turns         | locations |
            | a,g       | Oak St,Oak St | depart,arrive | a,g       |
            | g,a       | Oak St,Oak St | depart,arrive | g,a       |

    Scenario: Staggered Intersection: do not collapse if long segment in between
        Given the node map
            """
                j
            a b c
                |
                |
                d
                |
                |
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
            | waypoints | route                         | turns                              | locations |
            | a,g       | Oak St,Cedar Dr,Oak St,Oak St | depart,turn right,turn left,arrive | a,c,e,g   |
            | g,a       | Oak St,Cedar Dr,Oak St,Oak St | depart,turn right,turn left,arrive | g,e,c,a   |

    Scenario: Staggered Intersection: do not collapse if not left-right or right-left
        Given the node map
            """
                j
            a b c
                |
                d
                |
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
            | waypoints | route                | turns                        | locations |
            | a,g       | Oak St,Oak St,Oak St | depart,continue uturn,arrive | a,c,g     |
            | g,a       | Oak St,Oak St,Oak St | depart,continue uturn,arrive | g,e,a     |

    Scenario: Staggered Intersection: use new-name if the names are not the same
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
            | waypoints | route         | turns         | locations |
            | a,g       | Oak St,Elm St | depart,arrive | a,g       |
            | g,a       | Elm St,Oak St | depart,arrive | g,a       |

    Scenario: Staggered Intersection: do not collapse if a mode change is involved
        Given the node map
            """
                j
            a b c
                d
                e~~f - - - - g
                h
            """

        And the ways
            | nodes  | highway | name     | route |
            | abc    | primary | to_sea   |       |
            | ef     |         | to_sea   | ferry |
            | fg     | primary | road     |       |
            | jcdeh  | primary | road     |       |

        When I route I should get
            | waypoints | route                          | turns                                                    | modes                                 | locations |
            | a,g       | to_sea,road,to_sea,road,road   | depart,turn right,turn left,notification straight,arrive | driving,driving,ferry,driving,driving | a,c,e,f,g |
            | g,a       | road,to_sea,road,to_sea,to_sea | depart,notification straight,turn right,turn left,arrive | driving,ferry,driving,driving,driving | g,f,e,c,a |

    Scenario: Staggered Intersection: do not collapse intermediary intersections
        Given the node map
            """
                j
            a b c
                e f g
                |
                d
                |
                k l m
                i
            """

        And the ways
            | nodes   | highway     | name     |
            | abc     | primary     | Oak St   |
            | efg     | residential | Elm St   |
            | klm     | residential | Oak St   |
            | jcedki  | residential | Cedar Dr |

        When I route I should get
            | waypoints | route                         | turns                              | locations |
            | a,m       | Oak St,Cedar Dr,Oak St,Oak St | depart,turn right,turn left,arrive | a,c,k,m   |
            | m,a       | Oak St,Cedar Dr,Oak St,Oak St | depart,turn right,turn left,arrive | m,k,c,a   |
