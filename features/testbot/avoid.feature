@routing @testbot @avoid @mld
Feature: Testbot - Avoid flags
    Background:
        Given the profile "testbot"
        Given the node map
            """
            a....b-----c-$-$-d
                 $     $     :
                 e.$.$.f.....g
            """

        And the ways
            | nodes | highway  | toll | #                                                                  |
            | ab    | primary  |      | always drivable                                                    |
            | bc    | motorway |      | not drivable for avoid=motorway and avoid=motorway,toll            |
            | be    | primary  | yes  | not drivable for avoid=toll and avoid=motorway,toll                |
            | ef    | primary  | yes  | not drivable for avoid=toll and avoid=motorway,toll                |
            | fc    | primary  | yes  | not drivable for avoid=toll and avoid=motorway,toll                |
            | cd    | motorway | yes  | not drivable for avoid=motorway avoid=toll and avoid=motorway,toll |
            | fg    | primary  |      | always drivable                                                    |
            | gd    | primary  |      | always drivable                                                    |

    Scenario: Testbot - avoid nothing
        When I route I should get
            | from | to | route          |
            | a    | d  | ab,bc,cd,cd    |
            | a    | g  | ab,be,ef,fg,fg |
            | a    | c  | ab,bc,bc       |
            | a    | f  | ab,be,ef,ef    |

    Scenario: Testbot - avoid motorway
        Given the query options
            | avoid | motorway |

        When I route I should get
            | from | to | route             |
            | a    | d  | ab,be,ef,fg,gd,gd |
            | a    | g  | ab,be,ef,fg,fg    |
            | a    | c  | ab,be,ef,fc,fc    |
            | a    | f  | ab,be,ef,ef       |

    Scenario: Testbot - avoid toll
        Given the query options
            | avoid | toll |

        When I route I should get
            | from | to | route    |
            | a    | d  |          |
            | a    | g  |          |
            | a    | c  | ab,bc,bc |
            | a    | f  |          |
            | f    | d  | fg,gd,gd |

    Scenario: Testbot - avoid motorway and toll
        Given the query options
            | avoid | motorway,toll |

        When I route I should get
            | from | to | route    |
            | a    | d  |          |
            | a    | g  |          |
            | a    | c  | ab,bc,bc |
            | a    | f  |          |
            | f    | d  | fg,gd,gd |


