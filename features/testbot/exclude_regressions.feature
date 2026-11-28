@routing @testbot @exclude
Feature: Testbot - Exclude flags regression tests
    Background:
        Given the profile "testbot"

    Scenario: Testbot - Exclude toll regression 1
        Given the node map
            """
            a               g
            .               .
            b....d-$-$-e....f
            .               .
            c               h
            """

        And the ways
            | nodes | highway  | toll | #               |
            | ab    | primary  |      | always drivable |
            | cb    | primary  |      | always drivable |
            | bd    | primary  |      | always drivable |
            | de    | motorway | yes  | not drivable for exclude=toll |
            | ef    | primary  |      | always drivable |
            | fg    | primary  |      | always drivable |
            | fh    | primary  |      | always drivable |

        Given the query options
            | exclude | toll |

        When I route I should get
            | from | to | route         |
            | a    | h  |               |
            | a    | g  |               |
            | g    | a  |               |
            | d    | e  |               |

    Scenario: Testbot - Exclude toll regression 2
        Given the profile "testbot"

        Given the node map
            """
            a               g
            .               .
            b....d-$-$-e....f
            .               .
            c               h..i
            """

        And the ways
            | nodes | highway  | toll | #               |
            | ab    | primary  |      | always drivable |
            | cb    | primary  |      | always drivable |
            | bd    | primary  |      | always drivable |
            | de    | motorway | yes  | not drivable for exclude=toll |
            | ef    | primary  |      | always drivable |
            | fg    | primary  |      | always drivable |
            | fh    | primary  |      | always drivable |
            | hi    | primary  |      | always drivable |

        Given the query options
            | exclude | toll |

        When I route I should get
            | from | to | route         |
            | a    | h  |               |
            | a    | g  |               |
            | g    | a  |               |
            | d    | e  |               |
            | d    | i  |               |

    @with_mld @no_datastore
    Scenario: Testbot - MLD unpacking respects exclusions after an unrestricted route
        # Keep both requests on the same worker because the cache is thread-local.
        Given the data load extra arguments "--threads 1"
        And the partition extra arguments "--threads 1 --small-component-size 1 --max-cell-sizes 12,48,192"

        # The grid gives the middle cell two paths and enough graph data to cache one.
        Given the node locations
            | node | lon      | lat      |
            | a    | 1.000000 | 1.000000 |
            | b    | 1.001000 | 1.000000 |
            | c    | 1.002000 | 1.000000 |
            | d    | 1.003000 | 1.000000 |
            | e    | 1.004000 | 1.000000 |
            | f    | 1.000000 | 0.999000 |
            | g    | 1.001000 | 0.999000 |
            | h    | 1.002000 | 0.999000 |
            | i    | 1.003000 | 0.999000 |
            | j    | 1.004000 | 0.999000 |
            | k    | 1.000000 | 0.998000 |
            | l    | 1.001000 | 0.998000 |
            | m    | 1.002000 | 0.998000 |
            | n    | 1.003000 | 0.998000 |
            | o    | 1.004000 | 0.998000 |
            | p    | 1.000000 | 0.997000 |
            | q    | 1.001000 | 0.997000 |
            | r    | 1.002000 | 0.997000 |
            | s    | 1.003000 | 0.997000 |
            | t    | 1.004000 | 0.997000 |
            | u    | 1.000000 | 0.996000 |
            | v    | 1.001000 | 0.996000 |
            | w    | 1.002000 | 0.996000 |
            | x    | 1.003000 | 0.996000 |
            | y    | 1.004000 | 0.996000 |

        And the ways
            | nodes | toll | maxspeed |
            | ab    | yes  | 36       |
            | ag    | yes  | 36       |
            | bc    |      | 36       |
            | bg    | yes  | 36       |
            | bh    |      | 23       |
            | cd    |      | 36       |
            | ch    |      | 36       |
            | cg    |      | 23       |
            | de    |      | 36       |
            | di    | yes  | 36       |
            | ej    | yes  | 36       |
            | fg    |      | 36       |
            | fk    |      | 36       |
            | fl    |      | 23       |
            | gh    |      | 36       |
            | gl    |      | 36       |
            | gk    |      | 23       |
            | hi    |      | 36       |
            | hm    |      | 36       |
            | hn    | yes  | 36       |
            | hl    | yes  | 36       |
            | ij    |      | 36       |
            | in    |      | 36       |
            | im    |      | 23       |
            | jo    |      | 36       |
            | jn    |      | 23       |
            | kl    | yes  | 36       |
            | kp    |      | 36       |
            | kq    |      | 23       |
            | lm    |      | 36       |
            | lq    |      | 36       |
            | lr    | yes  | 36       |
            | mn    |      | 36       |
            | mr    |      | 36       |
            | ms    | yes  | 36       |
            | no    |      | 36       |
            | ns    |      | 36       |
            | ot    |      | 36       |
            | pq    |      | 36       |
            | pu    |      | 36       |
            | qr    |      | 36       |
            | qv    |      | 36       |
            | rs    |      | 36       |
            | rw    |      | 36       |
            | rv    | yes  | 36       |
            | st    |      | 36       |
            | sx    |      | 36       |
            | sy    |      | 23       |
            | sw    |      | 23       |
            | ty    |      | 36       |
            | tx    |      | 23       |
            | uv    |      | 36       |
            | vw    |      | 36       |
            | wx    |      | 36       |
            | xy    |      | 36       |

        When I route I should get
            | from | to | route                      |
            | e    | u  | de,cd,ch,hl,kl,kp,pu,pu    |

        Given the query options
            | exclude | toll |

        When I route I should get
            | from | to | route                      |
            | e    | u  | de,cd,ch,gh,fg,fk,kp,pu,pu |
