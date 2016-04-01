@routing @bicycle @area
Feature: Bike - Squares and other areas

    Background:
        Given the profile "bicycle"

    @square @mokob @2154
    Scenario: Bike - Route along edge of a squares
        Given the node map
            | x |   |
            | a | b |
            | d | c |

        And the ways
            | nodes | area | highway     |
            | xa    |      | primary     |
            | abcda | yes  | residential |

        When I route I should get
            | from | to | route       |
            | a    | b  | abcda,abcda |
            | a    | d  | abcda,abcda |
            | b    | c  | abcda,abcda |
            | c    | b  | abcda,abcda |
            | c    | d  | abcda,abcda |
            | d    | c  | abcda,abcda |
            | d    | a  | abcda,abcda |
            | a    | d  | abcda,abcda |

    @building
    Scenario: Bike - Don't route on buildings
        Given the node map
            | x |   |
            | a | b |
            | d | c |

        And the ways
            | nodes | highway | area | building | access |
            | xa    | primary |      |          |        |
            | abcda | (nil)   | yes  | yes      | yes    |

        When I route I should get
            | from | to | route |
            | a    | b  | xa    |
            | a    | d  | xa    |
            | b    | c  | xa    |
            | c    | b  | xa    |
            | c    | d  | xa    |
            | d    | c  | xa    |
            | d    | a  | xa    |
            | a    | d  | xa    |

    @parking @mokob @2154
    Scenario: Bike - parking areas
        Given the node map
            | e |   |   | f |
            | x | a | b | y |
            |   | d | c |   |

        And the ways
            | nodes | highway | amenity |
            | xa    | primary |         |
            | by    | primary |         |
            | xefy  | primary |         |
            | abcda | (nil)   | parking |

        When I route I should get
            | from | to | route          |
            | x    | y  | xa,abcda,by,by |
            | y    | x  | by,abcda,xa,xa |
            | a    | b  | abcda,abcda    |
            | a    | d  | abcda,abcda    |
            | b    | c  | abcda,abcda    |
            | c    | b  | abcda,abcda    |
            | c    | d  | abcda,abcda    |
            | d    | c  | abcda,abcda    |
            | d    | a  | abcda,abcda    |
            | a    | d  | abcda,abcda    |


    @train @platform @mokob @2154
    Scenario: Bike - railway platforms
        Given the node map
            | x | a | b | y |
            |   | d | c |   |

        And the ways
            | nodes | highway | railway  |
            | xa    | primary |          |
            | by    | primary |          |
            | abcda | (nil)   | platform |

        When I route I should get
            | from | to | route          |
            | x    | y  | xa,abcda,by,by |
            | y    | x  | by,abcda,xa,xa |
            | a    | b  | abcda,abcda    |
            | a    | d  | abcda,abcda    |
            | b    | c  | abcda,abcda    |
            | c    | b  | abcda,abcda    |
            | c    | d  | abcda,abcda    |
            | d    | c  | abcda,abcda    |
            | d    | a  | abcda,abcda    |
            | a    | d  | abcda,abcda    |
