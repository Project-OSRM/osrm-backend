@routing  @guidance @collapsing
Feature: Collapse

    Background:
        Given the profile "car"
        Given a grid size of 20 meters

    Scenario: Collapse Steps While On Ferry
        Given the node map
            """
            j----a---c---b----k
                  ~   ~  ~
                    ~  ~ ~
                       ~~~
                         d
                          ~
                           ~
                            ~
                             e --- f
            """

        And the ways
            | nodes | highway | route | name |
            | jacbk | primary |       | land |
            | ad    |         | ferry | sea  |
            | bd    |         | ferry | sea  |
            | cd    |         | ferry | sea  |
            | de    |         | ferry | sea  |
            | ef    | primary |       | land |

        When I route I should get
            | waypoints | route              | turns                                             | modes                         |
            | f,j       | land,sea,land,land | depart,notification right,end of road left,arrive | driving,ferry,driving,driving |

    Scenario: Switching Ferry in a Harbour
        Given the node map
            """
                          d
                          |
                          |
                          |
            e - a ~ ~ ~ ~ b
                          ~
                          ~
                          ~
                          c
                          |
                          f
            """

        And the ways
            | nodes | highway | route | name                  |
            | ea    | primary |       | melee-island          |
            | ab    |         | ferry | melee-island-ferry    |
            | cf    | primary |       | pennydog-island       |
            | bd    | primary |       | landmass              |
            | bc    | primary | ferry | pennydog-island-ferry |

        When I route I should get
            | waypoints | route                                                                                 | turns                                                                | modes                               |
            | e,f       | melee-island,melee-island-ferry,pennydog-island-ferry,pennydog-island,pennydog-island | depart,notification straight,turn right,notification straight,arrive | driving,ferry,ferry,driving,driving |

    Scenario: Fork Ferries
        Given the node map
            """
            a - b         d - e
                 ~       ~
                  ~     ~
                   ~   ~
                    ~ ~
                     c
                     ~
                     ~
                     ~
                     f
                     |
                     g
            """

        And the ways
            | nodes | highway | route | name        |
            | ab    | primary |       | land-left   |
            | de    | primary |       | land-right  |
            | gf    | primary |       | land-bottom |
            | cb    |         | ferry | ferry       |
            | cd    |         | ferry | ferry       |
            | fc    |         | ferry | ferry       |

        When I route I should get
            | waypoints | route                                   | turns                                                  |
            | g,e       | land-bottom,ferry,land-right,land-right | depart,notification straight,notification right,arrive |
