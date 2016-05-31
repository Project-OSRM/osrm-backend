@routing  @guidance
Feature: Mini Roundabout

    Background:
        Given the profile "car"
        Given a grid size of 10 meters

    Scenario: Enter and Exit mini roundabout
        Given the node map
            | a | b | c | d |

       And the ways
            | nodes | highway         | name |
            | ab    | tertiary        | MySt |
            | bc    | mini_roundabout |      |
            | cd    | tertiary        | MySt |

       When I route I should get
           | from | to | route     | turns         | #                                      |
           | a    | d  | MySt,MySt | depart,arrive | # suppress enter/exit mini roundabouts |

    Scenario: Enter and Exit subsequent mini roundabouts
        Given the node map
            | a | b | c | d | e |

       And the ways
            | nodes | highway         | name |
            | ab    | tertiary        | MySt |
            | bc    | mini_roundabout |      |
            | cd    | mini_roundabout |      |
            | de    | tertiary        | MySt |

       When I route I should get
           | from | to | route     | turns         | #                                               |
           | a    | e  | MySt,MySt | depart,arrive | # suppress multiple enter/exit mini roundabouts |

    Scenario: Enter and Exit mini roundabout with sharp angle
        Given the node map
            | a | b |   |
            |   | c | d |

       And the ways
            | nodes | highway         | name |
            | ab    | tertiary        | MySt |
            | bc    | mini_roundabout |      |
            | cd    | tertiary        | MySt |

       When I route I should get
           | from | to | route     | turns         | #                                               |
           | a    | d  | MySt,MySt | depart,arrive | # suppress multiple enter/exit mini roundabouts |

    Scenario: Enter and Exit mini roundabout with sharp angle
        Given the node map
            | a | b | e |
            |   | c | d |

       And the ways
            | nodes | highway         | name |
            | ab    | tertiary        | MySt |
            | bc    | mini_roundabout |      |
            | cd    | tertiary        | MySt |
            | be    | tertiary        | MySt |

       When I route I should get
           | from | to | route          | turns                    | #                                               |
           | a    | d  | MySt,MySt,MySt | depart,turn right,arrive | # suppress multiple enter/exit mini roundabouts |
