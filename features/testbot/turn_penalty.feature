@routing @testbot @turn_penalty
Feature: Turn Penalties

    Scenario: Turns should incur a delay that depend on the angle
        Given the profile "turnbot"
        Given a grid size of 200 meters
        Given the node map
            """
            c d e
            b j f
            a s g
            """

        And the ways
            | nodes |
            | sj    |
            | ja    |
            | jb    |
            | jc    |
            | jd    |
            | je    |
            | jf    |
            | jg    |

        When I route I should get
            | from | to | route    | time    | distance |
            | s    | a  | sj,ja,ja | 63s +-1 | 483m +-1 |
            | s    | b  | sj,jb,jb | 50s +-1 | 400m +-1 |
            | s    | c  | sj,jc,jc | 54s +-1 | 483m +-1 |
            | s    | d  | sj,jd,jd | 40s +-1 | 400m +-1 |
            | s    | e  | sj,je,je | 53s +-1 | 483m +-1 |
            | s    | f  | sj,jf,jf | 50s +-1 | 400m +-1 |
            | s    | g  | sj,jg,jg | 63s +-1 | 483m +-1 |
