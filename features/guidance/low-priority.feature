@routing  @guidance
Feature: Exceptions for routing onto low-priority roads

    Background:
        Given the profile "car"
        Given a grid size of 10 meters

    Scenario: Straight onto low-priority: same name
        Given the node map
            """
                c

            a   b d

                e
            """

        And the ways
            | nodes  | highway     | name    |
            | abd    | residential | road    |
            | eb     | service     | service |
            | bc     | service     | service |

       When I route I should get
            | waypoints | route            | turns         |
            | c,e       | service,service  | depart,arrive |
            | e,c       | service,service  | depart,arrive |

    Scenario: Straight onto low-priority: onto and from unnamed
        Given the node map
            """
                c

            a   b d

                e
            """

        And the ways
            | nodes  | highway     | name    |
            | abd    | residential | road    |
            | eb     | service     |         |
            | bc     | service     |         |

       When I route I should get
            | waypoints | route | turns         |
            | e,c       | ,     | depart,arrive |
            | c,e       | ,     | depart,arrive |

    Scenario: Straight onto low-priority: unnamed
        Given the node map
            """
                c

            a   b d

                e
            """

        And the ways
            | nodes  | highway     | name    |
            | abd    | residential | road    |
            | eb     | service     | service |
            | bc     | service     |         |

       When I route I should get
            | waypoints | route    | turns         |
            | e,c       | service, | depart,arrive |
            | c,e       | ,service | depart,arrive |

    Scenario: Straight onto low-priority
        Given the node map
            """
            a b c
            """

        And the ways
            | nodes  | highway     | name    |
            | ab     | residential | road    |
            | bc     | service     | service |

       When I route I should get
            | waypoints | route        | turns         |
            | a,c       | road,service | depart,arrive |

    Scenario: Straight onto low-priority, with driveway
        Given the node map
            """
                  f
            a b   c
            """

        And the ways
            | nodes  | highway     | name  |
            | ab     | residential | road  |
            | bc     | service     | road  |
            | bf     | driveway    |       |

       When I route I should get
            | waypoints | route      | turns         |
            | a,c       | road,road  | depart,arrive |

    Scenario: Straight onto low-priority, with driveway
        Given the node map
            """
                  f
            a b   c
            """

        And the ways
            | nodes  | highway     | name  |
            | ab     | residential | road  |
            | bc     | service     |       |
            | bf     | driveway    |       |

       When I route I should get
            | waypoints | route | turns         |
            | a,c       | road, | depart,arrive |
            | c,a       | ,road | depart,arrive |
