@routing @bicycle @alley
Feature: Bicycle - Route around alleys

    Background:
        Given the profile file "bicycle" initialized with
        """
        profile.properties.weight_name = 'cyclability'
        """
        
        Given the query options
            | annotations | weight,nodes |

    Scenario: Bicycle - Rate on alleys

        Given the node map
            """
            a---b
            c---d
            e---f
            """

        And the ways
            | nodes | highway     | service  |
            | ab    | residential |          |
            | cd    | service     | invalid  |
            | ef    | service     | alley    |

        When I route I should get
            | from | to | weight |
            | a    | b  | 48     |
            | c    | d  | 48     |
            | e    | f  | 96     |


    Scenario: Bicycle - Avoid taking alleys
        Given the node map
            """
            a-----b-----c
            |     :     |
            d.....e.....f
            """

        And the ways
            | nodes | highway     | service  |
            | abc   | residential |          |
            | def   | service     | alley    |
            | ad    | residential |          |
            | be    | service     | alley    |
            | cf    | residential |          |

        When I route I should get
            | from | to | a:nodes    | weight | #                                |
            | a    | d  | 1:4        | 56.3   |                                  |
            | b    | e  | 2:5        | 104.4  |                                  |
            | a    | f  | 1:2:3:6    | 200.4  | Avoids d,e,f                     |
            | a    | e  | 1:2:5      | 176.4  | Take the alley b,e if neccessary |
            | d    | f  | 4:1:2:3:6  | 252.6  | Avoids the alley d,e,f           |

