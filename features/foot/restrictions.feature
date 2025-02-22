@routing @foot @restrictions
Feature: Foot - Turn restrictions
# Ignore turn restrictions on foot.

    Background:
        Given the profile "foot"
        Given a grid size of 200 meters

    @no_turning
    Scenario: Foot - No left turn
        Given the node map
            """
              n
            w j e
              s
            """

        And the ways
            | nodes | oneway |
            | sj    | yes    |
            | nj    | -1     |
            | wj    | -1     |
            | ej    | -1     |

        And the relations
            | type        | way:from | way:to | node:via | restriction  |
            | restriction | sj       | wj     | j        | no_left_turn |

        When I route I should get
            | from | to | route    |
            | s    | w  | sj,wj,wj |
            | s    | n  | sj,nj    |
            | s    | e  | sj,ej,ej |

    @only_turning
    Scenario: Foot - Only left turn
        Given the node map
            """
              n
            w j e
              s
            """

        And the ways
            | nodes | oneway |
            | sj    | yes    |
            | nj    | -1     |
            | wj    | -1     |
            | ej    | -1     |

        And the relations
            | type        | way:from | way:to | node:via | restriction    |
            | restriction | sj       | wj     | j        | only_left_turn |

        When I route I should get
            | from | to | route    |
            | s    | w  | sj,wj,wj |
            | s    | n  | sj,nj    |
            | s    | e  | sj,ej,ej |

    @except
    Scenario: Foot - Except tag and on no_ restrictions
        Given the node map
            """
            b x c
            a j d
              s
            """

        And the ways
            | nodes | oneway |
            | sj    | no     |
            | xj    | -1     |
            | aj    | -1     |
            | bj    | no     |
            | cj    | -1     |
            | dj    | -1     |

        And the relations
            | type        | way:from | way:to | node:via | restriction   | except |
            | restriction | sj       | aj     | j        | no_left_turn  | foot   |
            | restriction | sj       | bj     | j        | no_left_turn  |        |
            | restriction | sj       | cj     | j        | no_right_turn |        |
            | restriction | sj       | dj     | j        | no_right_turn | foot   |

        When I route I should get
            | from | to | route    |
            | s    | a  | sj,aj,aj |
            | s    | b  | sj,bj,bj |
            | s    | c  | sj,cj,cj |
            | s    | d  | sj,dj,dj |

    @except
    Scenario: Foot - Multiple except tag values
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
            | nodes | oneway |
            | sj    | yes    |
            | ja    | yes    |
            | jb    | yes    |
            | jc    | yes    |
            | jd    | yes    |
            | je    | yes    |
            | jf    | yes    |

        And the relations
            | type        | way:from | way:to | node:via | restriction    | except        |
            | restriction | sj       | ja     | j        | no_straight_on |               |
            | restriction | sj       | jb     | j        | no_straight_on | foot          |
            | restriction | sj       | jc     | j        | no_straight_on | bus; foot     |
            | restriction | sj       | jd     | j        | no_straight_on | foot; motocar |
            | restriction | sj       | je     | j        | no_straight_on | bus, foot     |
            | restriction | sj       | jf     | j        | no_straight_on | foot, bus     |

        When I route I should get
            | from | to | route    |
            | s    | a  | sj,ja,ja |
            | s    | b  | sj,jb,jb |
            | s    | c  | sj,jc,jc |
            | s    | d  | sj,jd,jd |
            | s    | e  | sj,je,je |
            | s    | f  | sj,jf,jf |
