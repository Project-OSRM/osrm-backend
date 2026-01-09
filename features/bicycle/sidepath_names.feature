@routing @bicycle @sidepath
Feature: Bicycle - Sidepath street names

    Background:
        Given the profile "bicycle"
        Given a grid size of 200 meters

    Scenario: Bicycle - Use is_sidepath:of:name for cycleway sidepath
        Given the node map
            """
            a b c
            """

        And the ways
            | nodes | highway  | cycleway | name | is_sidepath:of:name |
            | ab    | cycleway | sidepath |      | Highway 1           |
            | bc    | cycleway | sidepath |      | Highway 2           |

        When I route I should get
            | from | to | route                        |
            | a    | c  | Highway 1,Highway 2,Highway 2 |

    Scenario: Bicycle - Use street:name for cycleway sidepath
        Given the node map
            """
            a b
            """

        And the ways
            | nodes | highway  | cycleway | name | street:name |
            | ab    | cycleway | sidepath |      | Bike Lane   |

        When I route I should get
            | from | to | route               |
            | a    | b  | Bike Lane,Bike Lane |

    Scenario: Bicycle - Explicit name takes priority
        Given the node map
            """
            a b
            """

        And the ways
            | nodes | highway  | cycleway | name       | is_sidepath:of:name |
            | ab    | cycleway | sidepath | Named Path | Fallback            |

        When I route I should get
            | from | to | route                 |
            | a    | b  | Named Path,Named Path |

    Scenario: Bicycle - Use is_sidepath=yes with is_sidepath:of:name
        Given the node map
            """
            a b
            """

        And the ways
            | nodes | highway | is_sidepath | name | is_sidepath:of:name |
            | ab    | path    | yes         |      | Cycle Route         |

        When I route I should get
            | from | to | route                   |
            | a    | b  | Cycle Route,Cycle Route |

    Scenario: Bicycle - No fallback without sidepath marker
        Given the node map
            """
            a b
            """

        And the ways
            | nodes | highway  | name | street:name     |
            | ab    | cycleway |      | Should Not Show |

        When I route I should get
            | from | to | route |
            | a    | b  | ,     |
