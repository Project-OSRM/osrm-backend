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

    Scenario: Car - Height restrictions
        Then routability should be
            | node/barrier      | node/maxheight | bothw |
            | height_restrictor |                | x     |
            | height_restrictor |              1 |       |
            | height_restrictor |              3 | x     |
            | height_restrictor |        default | x     |
