@routing @bicycle @turn_penalty
Feature: Turn Penalties

    Background:
        Given the profile "turnbot"
        Given a grid size of 200 meters

    Scenario: Bike - turns should incur a delay that depend on the angle

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

    Scenario: Bicycle - Turn penalties on cyclability
        Given the profile file
        """
        require 'bicycle'
        properties.weight_name = 'cyclability'
        """

        Given the node map
            """
            a--b-----c
               |
               |
               d

            e--------f-----------g
                  /
                /
              /
            h
            """

        And the ways
            | nodes | highway     |
            | abc   | residential |
            | bd    | residential |
            | efg   | residential |
            | fh    | residential |

        When I route I should get
            | from | to | distance  | weight | #                                         |
            | a    | c  | 900m +- 1 | 216    | Going straight has no penalties           |
            | a    | d  | 900m +- 1 | 220.2  | Turning right had penalties               |
            | e    | g  | 2100m +- 4| 503.9  | Going straght has no penalties            |
            | e    | h  | 2100m +- 4| 515.1  | Turn sharp right has even higher penalties|

