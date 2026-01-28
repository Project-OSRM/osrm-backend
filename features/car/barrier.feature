@routing @car @barrier
Feature: Car - Barriers

    Background:
        Given the profile "car"

    Scenario: Car - Barriers
        Then routability should be
            | node/barrier   | bothw |
            |                | x     |
            | bollard        |       |
            | gate           | x     |
            | lift_gate      | x     |
            | cattle_grid    | x     |
            | border_control | x     |
            | toll_booth     | x     |
            | sally_port     | x     |
            | entrance       | x     |
            | wall           |       |
            | fence          |       |
            | some_tag       |       |
            | block          |       |

    Scenario: Car - Access tag trumphs barriers
        Then routability should be
            | node/barrier | node/access   | bothw |
            | gate         |               | x     |
            | gate         | yes           | x     |
            | gate         | permissive    | x     |
            | gate         | designated    | x     |
            | gate         | no            |       |
            | gate         | private       | x     |
            | gate         | agricultural  |       |
            | lift_gate    |               | x     |
            | lift_gate    | yes           | x     |
            | lift_gate    | permissive    | x     |
            | lift_gate    | designated    | x     |
            | lift_gate    | no            |       |
            | lift_gate    | private       | x     |
            | lift_gate    | agricultural  |       |
            | wall         |               |       |
            | wall         | yes           | x     |
            | wall         | permissive    | x     |
            | wall         | designated    | x     |
            | wall         | no            |       |
            | wall         | private       | x     |
            | wall         | agricultural  |       |

    Scenario: Car - Rising bollard exception for barriers
        Then routability should be
            | node/barrier | node/bollard  | bothw |
            | bollard      |               |       |
            | bollard      | rising        | x     |
            | bollard      | removable     |       |

    # https://github.com/Project-OSRM/osrm-backend/issues/5996
    Scenario: Car - Kerb exception for barriers
        Then routability should be
            | node/barrier | node/highway  | node/kerb  | bothw |
            | kerb         |               |            |       |
            | kerb         | crossing      |            |  x    |
            | kerb         | crossing      | yes        |  x    |
            | kerb         |               | lowered    |  x    |
            | kerb         |               | flush      |  x    |
            | kerb         |               | raised     |       |
            | kerb         |               | yes        |       |

    Scenario: Car - Audible fence exception for barriers
        # Audible fences use sound to deter livestock but do not block vehicles
        Then routability should be
            | node/barrier | node/sensory | bothw |
            | fence        |              |       |
            | fence        | audible      | x     |
            | fence        | audio        | x     |

    Scenario: Car - Height restrictions
        Then routability should be
            | node/barrier      | node/maxheight | bothw |
            | height_restrictor |                | x     |
            | height_restrictor |              1 |       |
            | height_restrictor |              3 | x     |
            | height_restrictor |        default | x     |

    Scenario: Car - Gate penalties
        Given the node map
            """
            a-b-c   d-e-f   g-h-i
            """

        And the ways
            | nodes | highway |
            | abc   | primary |
            | def   | primary |
            | ghi   | primary |

        And the nodes
            | node | barrier   |
            | e    | gate      |
            | h    | lift_gate |

        When I route I should get
            | from | to | time    | #                 |
            | a    | c  | 11s     | no barrier        |
            | d    | f  | 71s +-1 | gate penalty      |
            | g    | i  | 71s +-1 | lift_gate penalty |


    Scenario: Car - Gate penalty skipped with explicit access tag
        Given the node map
            """
            a-b-c   d-e-f
            """

        And the ways
            | nodes | highway |
            | abc   | primary |
            | def   | primary |

        And the nodes
            | node | barrier | access |
            | b    | gate    |        |
            | e    | gate    | yes    |

        When I route I should get
            | from | to | time    | #                                    |
            | a    | c  | 71s +-1 | gate without access gets penalty     |
            | d    | f  | 11s     | gate with access=yes skips penalty   |


