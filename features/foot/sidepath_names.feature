@routing @foot @sidepath
Feature: Foot - Sidepath street names

    Background:
        Given the profile "foot"
        Given a grid size of 200 meters

    Scenario: Foot - Use is_sidepath:of:name for unnamed sidewalk
        Given the node map
            """
            a b c
            """

        And the ways
            | nodes | highway | footway  | name | is_sidepath:of:name |
            | ab    | footway | sidewalk |      | Main Street         |
            | bc    | footway | sidewalk |      | Oak Avenue          |

        When I route I should get
            | from | to | route                             |
            | a    | c  | Main Street,Oak Avenue,Oak Avenue |

    Scenario: Foot - Use street:name for unnamed sidewalk
        Given the node map
            """
            a b c
            """

        And the ways
            | nodes | highway | footway  | name | street:name |
            | ab    | footway | sidewalk |      | Elm Street  |
            | bc    | footway | sidewalk |      | Pine Road   |

        When I route I should get
            | from | to | route                           |
            | a    | c  | Elm Street,Pine Road,Pine Road  |

    Scenario: Foot - is_sidepath:of:name takes priority over street:name
        Given the node map
            """
            a b
            """

        And the ways
            | nodes | highway | footway  | name | is_sidepath:of:name | street:name |
            | ab    | footway | sidewalk |      | Primary Name        | Secondary   |

        When I route I should get
            | from | to | route                       |
            | a    | b  | Primary Name,Primary Name   |

    Scenario: Foot - Explicit name tag takes priority over fallback
        Given the node map
            """
            a b
            """

        And the ways
            | nodes | highway | footway  | name          | is_sidepath:of:name |
            | ab    | footway | sidewalk | Explicit Name | Fallback Name       |

        When I route I should get
            | from | to | route                         |
            | a    | b  | Explicit Name,Explicit Name   |

    Scenario: Foot - Use is_sidepath=yes with street:name
        Given the node map
            """
            a b
            """

        And the ways
            | nodes | highway | is_sidepath | name | street:name |
            | ab    | path    | yes         |      | River Path  |

        When I route I should get
            | from | to | route                   |
            | a    | b  | River Path,River Path   |

    Scenario: Foot - No fallback without sidepath marker
        Given the node map
            """
            a b
            """

        And the ways
            | nodes | highway | name | street:name     |
            | ab    | footway |      | Should Not Show |

        When I route I should get
            | from | to | route |
            | a    | b  | ,     |
