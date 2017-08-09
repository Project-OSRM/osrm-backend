@routing @bicycle @restrictions
Feature: Bike - Turn restrictions
# Ignore turn restrictions on bicycle, since you always become a temporary pedestrian.
# Note that if u-turns are allowed, turn restrictions can lead to suprising, but correct, routes.

    Background:
        Given the profile "bicycle"
        Given a grid size of 200 meters

    # check to see if bicycle profiles handle relations. The code path is the same as in cars.
    # If relations are passed, all variants tested in car also work for bike
    @no_turning
    Scenario: Bike - No left turn
        Given the node map
            """
              n
            w j e
              s
            """

        And the ways
            | nodes | oneway | foot |
            | sj    | yes    | no   |
            | nj    | -1     | no   |
            | wj    | -1     | no   |
            | ej    | -1     | no   |

        And the relations
            | type        | way:from | way:to | node:via | restriction  |
            | restriction | sj       | wj     | j        | no_left_turn |

        When I route I should get
            | from | to | route    |
            | s    | w  | sj,wj,wj |
            | s    | n  | sj,nj,nj |
            | s    | e  | sj,ej,ej |

    ## exceptions are different for bike over car and need to be tested here
    @except
    Scenario: Bike - Except tag and on no_ restrictions
        Given the node map
            """
            b x c
            a j d
              s
            """

        And the ways
            | nodes | oneway | foot |
            | sj    | no     | no   |
            | xj    | -1     | no   |
            | aj    | -1     | no   |
            | bj    | no     | no   |
            | cj    | -1     | no   |
            | dj    | -1     | no   |

        And the relations
            | type        | way:from | way:to | node:via | restriction   | except  |
            | restriction | sj       | aj     | j        | no_left_turn  | bicycle |
            | restriction | sj       | bj     | j        | no_left_turn  |         |
            | restriction | sj       | cj     | j        | no_right_turn |         |
            | restriction | sj       | dj     | j        | no_right_turn | bicycle |

        When I route I should get
            | from | to | route    |
            | s    | a  | sj,aj,aj |
            | s    | b  | sj,bj,bj |
            | s    | c  | sj,cj,cj |
            | s    | d  | sj,dj,dj |

    @except
    Scenario: Bike - Except tag and on only_ restrictions
        Given the node map
            """
            a   b
              j
              s
            """

        And the ways
            | nodes | oneway | foot |
            | sj    | yes    | no   |
            | aj    | no     | no   |
            | bj    | no     | no   |

        And the relations
            | type        | way:from | way:to | node:via | restriction      | except  |
            | restriction | sj       | aj     | j        | only_straight_on | bicycle |

        When I route I should get
            | from | to | route    |
            | s    | a  | sj,aj,aj |
            | s    | b  | sj,bj,bj |

    @except
    Scenario: Bike - Multiple except tag values
        Given the node map
            """
            s j a
                b
                c
                d
                e
                f
            """

        And the ways
            | nodes | oneway | foot |
            | sj    | yes    | no   |
            | ja    | yes    | no   |
            | jb    | yes    | no   |
            | jc    | yes    | no   |
            | jd    | yes    | no   |
            | je    | yes    | no   |
            | jf    | yes    | no   |

        And the relations
            | type        | way:from | way:to | node:via | restriction    | except           |
            | restriction | sj       | ja     | j        | no_straight_on |                  |
            | restriction | sj       | jb     | j        | no_straight_on | bicycle          |
            | restriction | sj       | jc     | j        | no_straight_on | bus; bicycle     |
            | restriction | sj       | jd     | j        | no_straight_on | bicycle; motocar |
            | restriction | sj       | je     | j        | no_straight_on | bus, bicycle     |
            | restriction | sj       | jf     | j        | no_straight_on | bicycle, bus     |

        When I route I should get
            | from | to | route    |
            | s    | a  | sj,ja,ja |
            | s    | b  | sj,jb,jb |
            | s    | c  | sj,jc,jc |
            | s    | d  | sj,jd,jd |
            | s    | e  | sj,je,je |
            | s    | f  | sj,jf,jf |
