@routing @guidance @mode-change
Feature: Notification on turn onto mode change

    Background:
        Given the profile "car"
        Given a grid size of 400 meters

    Scenario: Turn onto a Ferry
        Given the node map
            """
            f
            b     d
            a       e
            """

        And the ways
            | nodes | highway | route | name  |
            | abf   | primary |       |       |
            | bd    |         | ferry | ferry |
            | de    | primary |       |       |

        When I route I should get
            | waypoints | route     | turns                                        | modes                          |
            | a,e       | ,ferry,,  | depart,turn right,notification right,arrive  | driving,ferry,driving,driving  |

    Scenario: Turn onto a Ferry
        Given the node map
            """
            h     g
            a c   e
            b     f
            """

        And the ways
            | nodes | highway | route | name  |
            | ac    | primary |       |       |
            | bah   | primary |       |       |
            | ec    |         | ferry | ferry |
            | gef   | primary |       |       |

        When I route I should get
            | waypoints | route     | turns                                                      | modes                                   |
            | g,h       | ,ferry,,, | depart,turn right,notification straight,turn right,arrive  | driving,ferry,driving,driving,driving   |
            | b,g       | ,,ferry,, | depart,turn right,notification straight,turn left,arrive   | driving,driving,ferry,driving,driving   |

    Scenario: Straight onto a Ferry
        Given the node map
            """

              c d   i
            a
                  f
            """

        And the ways
            | nodes | highway | route | name    |
            | ac    | primary |       |         |
            | dc    |         | ferry | ferry   |
            | df    | primary |       |         |

        When I route I should get
            | waypoints | route     | turns                                                  | modes                           |
            | a,f       | ,ferry,,  | depart,notification right,notification right,arrive    | driving,ferry,driving,driving   |
