@routing @car @restrictions
Feature: Car - Turn restrictions
# Handle turn restrictions as defined by http://wiki.openstreetmap.org/wiki/Relation:restriction
# Note that if u-turns are allowed, turn restrictions can lead to suprising, but correct, routes.

    Background: Use car routing
        Given the profile "car"
        Given a grid size of 200 meters

    @only_turning @conditionals
    Scenario: Car - ignores unrecognized restriction
        Given the extract extra arguments "--parse-conditional-restrictions=1"
        Given the contract extra arguments "--time-zone-file=test/data/tz_world.shp"
        Given the node map
            """
              n
            p j e
              s
            """

        And the ways
            | nodes | oneway |
            | nj    | no     |
            | js    | no     |
            | ej    | yes    |
            | jp    | yes    |

        And the relations
            | type        | way:from | way:to | node:via | restriction:conditional           |
            | restriction | ej       | nj     | j        | only_right_turn @ (length > 10 m) |

        When I route I should get
            | from | to | route    |
            | e    | s  | ej,js,js |
            | e    | n  | ej,nj,nj |
            | e    | p  | ej,jp,jp |

    @only_turning @conditionals
    Scenario: Car - only_right_turn
        Given the extract extra arguments "--parse-conditional-restrictions=1"
        Given the contract extra arguments "--time-zone-file=test/data/tz_world.shp"
        Given the node map
            """
              n
            p j e
              s
            """

        And the ways
            | nodes | oneway |
            | nj    | no     |
            | js    | no     |
            | ej    | yes    |
            | jp    | yes    |

        And the relations
            | type        | way:from | way:to | node:via | restriction:conditional               |
            | restriction | ej       | nj     | j        | only_right_turn @ (Mo-Su 00:00-23:59) |

        When I route I should get
            | from | to | route          |
            | e    | s  | ej,nj,nj,js,js |
            | e    | n  | ej,nj,nj       |
            | e    | p  | ej,nj,nj,jp,jp |

    @no_turning @conditionals
    Scenario: Car - No right turn
        Given the extract extra arguments "--parse-conditional-restrictions=1"
        Given the contract extra arguments "--time-zone-file=test/data/tz_world.shp"
        Given the node map
            """
              n
            p j e
              s
            """

        And the ways
            | nodes | oneway |
            | nj    | no     |
            | js    | no     |
            | ej    | yes    |
            | jp    | yes    |

        And the relations
            | type        | way:from | way:to | node:via | restriction:conditional               |
            | restriction | ej       | nj     | j        | no_right_turn @ (Mo-Su 00:00-23:59) |

        When I route I should get
            | from | to | route          |
            | e    | s  | ej,js,js       |
            | e    | n  | ej,js,js,nj,nj |
            | e    | p  | ej,jp,jp       |

    @only_turning @conditionals
    Scenario: Car - only_left_turn
        Given the extract extra arguments "--parse-conditional-restrictions=1"
        Given the contract extra arguments "--time-zone-file=test/data/tz_world.shp"
        Given the node map
            """
              n
            p j e
              s
            """

        And the ways
            | nodes | oneway |
            | nj    | no     |
            | js    | no     |
            | ej    | yes    |
            | jp    | yes    |

        And the relations
            | type        | way:from | way:to | node:via | restriction:conditional               |
            | restriction | ej       | js     | j        | only_left_turn @ (Mo-Su 00:00-23:59) |

        When I route I should get
            | from | to | route          |
            | e    | s  | ej,js,js       |
            | e    | n  | ej,js,js,nj,nj |
            | e    | p  | ej,js,js,jp,jp |

    @no_turning @conditionals
    Scenario: Car - No left turn
        Given the extract extra arguments "--parse-conditional-restrictions=1"
        Given the contract extra arguments "--time-zone-file=test/data/tz_world.shp"
        Given the node map
            """
              n
            p j e
              s
            """

        And the ways
            | nodes | oneway |
            | nj    | no     |
            | js    | no     |
            | ej    | yes    |
            | jp    | yes    |

        And the relations
            | type        | way:from | way:to | node:via | restriction:conditional            |
            | restriction | ej       | js     | j        | no_left_turn @ (Mo-Su 00:00-23:59) |

        When I route I should get
            | from | to | route          |
            | e    | s  | ej,nj,nj,js,js |
            | e    | n  | ej,nj,nj       |
            | e    | p  | ej,jp,jp       |

    @no_turning @conditionals
    Scenario: Car - Conditional restriction is off
        Given the extract extra arguments "--parse-conditional-restrictions=1"
        Given the contract extra arguments "--time-zone-file=test/data/tz_world.shp"
        Given the node map
            """
              n
            p j e
              s
            """

        And the ways
            | nodes | oneway |
            | nj    | no     |
            | js    | no     |
            | ej    | yes    |
            | jp    | yes    |

        And the relations
            | type        | way:from | way:to | node:via | restriction:conditional               |
            | restriction | ej       | nj     | j        | no_right_turn @ (Mo-Su 00:00-00:00)   |

        When I route I should get
            | from | to | route    |
            | e    | s  | ej,js,js |
            | e    | n  | ej,nj,nj |
            | e    | p  | ej,jp,jp |
