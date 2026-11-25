@routing @guidance @bicycle
Feature: Bicycle roundabout exit numbering

    Background:
        Given a grid size of 30 meters

    Scenario Outline: Pass an inbound one-way arm on the roundabout
        Given the profile "<profile>"
        Given the node map
            """
                a
                b
            d c   g h
                e
                f
            """

        And the ways
            | nodes | name | highway     | junction   | oneway | oneway:bicycle |
            | ab    | ab   | residential |            | yes    |                |
            | dc    | dc   | residential |            | yes    | <contraflow>   |
            | ef    | ef   | residential |            | yes    |                |
            | gh    | gh   | residential |            | yes    |                |
            | bcegb |      | residential | roundabout |        |                |

        When I route I should get
            | waypoints | route       | turns                                                        |
            | a,f       | ab,ef,ef,ef | depart,roundabout-exit-<first>,exit roundabout right,arrive  |
            | a,h       | ab,gh,gh,gh | depart,roundabout-exit-<second>,exit roundabout right,arrive |

        Examples:
            | profile | contraflow | first | second |
            | bicycle |            | 1     | 2      |
            | bicycle | no         | 2     | 3      |
            | car     |            | 1     | 2      |

    Scenario: Take an exit that requires pushing the bicycle
        Given the profile "bicycle"
        Given the node map
            """
                a

                b
               1
            d c   g h

                e

                f
            """

        And the ways
            | nodes | name | highway     | junction   | oneway |
            | ab    | ab   | residential |            | yes    |
            | dc    | dc   | residential |            | yes    |
            | ef    | ef   | residential |            | yes    |
            | gh    | gh   | residential |            | yes    |
            | bcegb |      | residential | roundabout |        |

        When I route I should get
            | waypoints | route       | turns                                                         | modes                                     |
            | a,d       | ab,dc,dc,dc | depart,roundabout-exit-undefined,exit roundabout right,arrive | cycling,cycling,pushing bike,pushing bike |
            | 1,d       | ,dc,dc      | depart,exit roundabout right,arrive                           | cycling,pushing bike,pushing bike         |

    Scenario: Pass a pushing-only arm at the roundabout entry junction
        Given the profile "bicycle"
        Given the node map
            """
              c   a
            j   b   f
              k   e
            l   h   d
              g   i
            """

        And the ways
            | nodes | name | highway     | junction   | oneway |
            | ab    | ab   | residential |            | yes    |
            | cb    | cb   | residential |            | yes    |
            | def   | def  | residential |            | yes    |
            | ghi   | ghi  | residential |            | yes    |
            | jkl   | jkl  | residential |            | yes    |
            | bkheb |      | residential | roundabout | yes    |

        When I route I should get
            | waypoints | route          | turns                                                    | modes                             |
            | a,l       | ab,jkl,jkl,jkl | depart,roundabout-exit-1,exit roundabout straight,arrive | cycling,cycling,cycling,cycling   |
            | a,c       | ab,cb,cb       | depart,exit roundabout right,arrive                      | cycling,pushing bike,pushing bike |
