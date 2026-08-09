@routing @foot @area
Feature: Foot - Pedestrian areas

    Background:
        Given the profile "foot_area"
        Given a grid size of 50 meters
        Given the extract extra arguments "--verbosity DEBUG"
        Given the query options
            | annotations | nodes |

    Scenario: Foot - Route across a closed way area
        Given the node map
            """
            e-a---b-f
              |   |
            h-d---c-g
            """

        And the ways
            | nodes | highway    | area | name  |
            | abcda | pedestrian | yes  | Plaza |
            | ea    | pedestrian |      | A     |
            | bf    | pedestrian |      | B     |
            | hd    | pedestrian |      | C     |
            | cg    | pedestrian |      | D     |

        When I route I should get
            | from | to | a:nodes | route     |
            | e    | g  | eacg    | A,Plaza,D |
            | g    | e  | gcae    | D,Plaza,A |
            | h    | f  | hdbf    | C,Plaza,B |
            | f    | h  | fbdh    | B,Plaza,C |

    Scenario: Foot - Do not route across a closed way w/o area
        Given the node map
            """
            e-a--b-f
              |   \
            h-d---c-g
            """

        And the ways
            | nodes | highway    |
            | abcda | pedestrian |
            | ea    | pedestrian |
            | bf    | pedestrian |
            | hd    | pedestrian |
            | cg    | pedestrian |

        When I route I should get
            | from | to | a:nodes |
            | e    | g  | eabcg   |
            | g    | e  | gcbae   |
            | h    | f  | hdabf   |
            | f    | h  | fbadh   |

    Scenario: Foot - Route across a multipolygon area
        Given the node map
            """
            e-a-------b-f
              | u-v   |
              | x-w   |
            h-d-------c-g
            """

        And the ways
            | nodes | highway    | name |
            | abcda | (nil)      |      |
            | uvwxu | (nil)      |      |
            | ea    | pedestrian | A    |
            | bf    | pedestrian | B    |
            | hd    | pedestrian | C    |
            | cg    | pedestrian | D    |

        And the relations
            | type         | highway    | name  | way:outer | way:inner |
            | multipolygon | pedestrian | Plaza | abcda     | uvwxu     |

        When I route I should get
            | from | to | a:nodes | route     |
            | e    | g  | eavcg   | A,Plaza,D |
            | g    | e  | gcvae   | D,Plaza,A |
            | f    | h  | fbwdh   | B,Plaza,C |
            | h    | f  | hdwbf   | C,Plaza,B |

    Scenario: Foot - Route across a complex multipolygon area
        Given the node map
            """
            g-a---------------b-h
              | z-y           |
              | | |           |
            l-f | |      v--u |
              | w-x      |  | |
              |          |  | c-i
              |          |  | |
              |          s--t |
            k-e---------------d-j
            """

        And the ways
            | nodes   | highway    |
            | abcdefa | (nil)      |
            | zwxyz   | (nil)      |
            | vstuv   | (nil)      |
            | ag      | pedestrian |
            | bh      | pedestrian |
            | ci      | pedestrian |
            | dj      | pedestrian |
            | ek      | pedestrian |
            | fl      | pedestrian |

        And the relations
            | type         | highway    | way:outer | way:inner   |
            | multipolygon | pedestrian | abcdefa   | vstuv,zwxyz |

        When I route I should get
            | from | to | a:nodes  |
            | g    | h  | gabh     |
            | g    | i  | gauci    |
            | g    | j  | gaysdj   |
            | l    | h  | lfzbh    |
            | l    | i  | lfwxvuci |
            | l    | j  | lfwsdj   |
            | k    | h  | kebh     |
            | k    | i  | ketci    |
            | k    | j  | kedj     |

    # Two ways in is the smallest arrangement worth meshing.  A plaza entered twice from
    # the same side has no other graph in it at all, so refusing to mesh it leaves the
    # two entrances unconnected except by whatever runs around the outside.
    Scenario: Foot - Mesh an area with only two entrances
        Given the node map
            """
            a-------b
            |       |
            d-x---y-c
              |   |
              u   v
            """

        And the ways
            | nodes    | highway    | area | name  |
            | abcyxda  | pedestrian | yes  | Plaza |
            | ux       | pedestrian |      | U     |
            | vy       | pedestrian |      | V     |

        # the chord x-y is the mesh; without it the two entrances are not connected
        When I route I should get
            | from | to | a:nodes | distance |
            | u    | v  | uxyv    | 300m +-2 |
            | v    | u  | vyxu    | 300m +-2 |

    # ... but one way touching the area is not an arrangement at all: there is nothing to
    # cross to, so the mesh would be dead weight.
    Scenario: Foot - Do not mesh an area with a single entrance
        Given the node map
            """
            a-------b
            |       |
            d-x-----c
              |
              u
            """

        And the ways
            | nodes   | highway    | area | name  |
            | abcxda  | pedestrian | yes  | Plaza |
            | ux      | pedestrian |      | U     |

        When I route I should get
            | from | to | route |
            | u    | x  | U,U   |
