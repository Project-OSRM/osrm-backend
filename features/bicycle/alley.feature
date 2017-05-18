@routing @bicycle @alley
Feature: Bicycle - Route around alleys

    Background:
        Given the profile file "bicycle" initialized with
        """
        profile.properties.weight_name = 'cyclability'
        """

    Scenario: Bicycle - Avoid taking alleys
        Given the query options
            | annotations | nodes |

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
            | a    | f  | 1:2:3:6    | 200.4  | Avoids d,e,f                     |
            | a    | e  | 1:2:5      | 176.4  | Take the alley b,e if neccessary |
            | d    | f  | 4:1:2:3:6  | 252.6  | Avoids the alley d,e,f           |

