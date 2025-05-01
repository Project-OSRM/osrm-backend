@routing @foot @area
Feature: Foot - Pedestrian areas

    Background:
        Given the profile "foot_area"
        Given a grid size of 50 meters
        # Given the extract extra arguments "--verbosity DEBUG"
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
            | nodes | highway    | area |
            | abcda | pedestrian | yes  |
            | ea    | pedestrian |      |
            | bf    | pedestrian |      |
            | hd    | pedestrian |      |
            | cg    | pedestrian |      |

        When I route I should get
            | from | to | a:nodes |
            | e    | g  | eacg    |
            | g    | e  | gcae    |
            | h    | f  | hdbf    |
            | f    | h  | fbdh    |

    Scenario: Foot - Do not route across a closed way w/o area
        Given the node map
            """
            e-a---b-f
              |   |
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
            | nodes | highway    |
            | abcda | (nil)      |
            | uvwxu | (nil)      |
            | ea    | pedestrian |
            | bf    | pedestrian |
            | hd    | pedestrian |
            | cg    | pedestrian |

        And the relations
            | type         | highway    | way:outer | way:inner |
            | multipolygon | pedestrian | abcda     | uvwxu     |

        When I route I should get
            | from | to | a:nodes |
            | e    | g  | eavcg   |
            | g    | e  | gcvae   |
            | f    | h  | fbwdh   |
            | h    | f  | hdwbf   |

    Scenario: Foot - Route across a complex multipolygon area
        Given the node map
            """
            g-a---------------b-h
              | z-y           |
              | | |           |
            l-f | |           |
              | w-x       v-u |
              |           | | c-i
              |           | | |
              |           s-t |
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
            | from | to | a:nodes  | #                            |
            | g    | h  | gabh     |                              |
            | g    | i  | gayuci   |                              |
            | g    | j  | gaysdj   |                              |
            | l    | h  | lfzybh   | use existing segment zy      |
            | l    | i  | lfwxvuci | use existing segments wx, vu |
            | l    | j  | lfwsdj   |                              |
            | k    | h  | kebh     |                              |
            | k    | i  | kestci   | use existing segment st      |
            | k    | j  | kedj     |                              |
