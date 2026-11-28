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
            | nodes | name | highway   | junction   | oneway | oneway:bicycle |
            | ab    | ab   | <highway> |            | yes    |                |
            | dc    | dc   | <highway> |            | yes    | <contraflow>   |
            | ef    | ef   | <highway> |            | yes    |                |
            | gh    | gh   | <highway> |            | yes    |                |
            | bcegb |      | <highway> | roundabout |        |                |

        When I route I should get
            | waypoints | route       | turns                                                        |
            | a,f       | ab,ef,ef,ef | depart,roundabout-exit-<first>,exit roundabout right,arrive  |
            | a,h       | ab,gh,gh,gh | depart,roundabout-exit-<second>,exit roundabout right,arrive |

        Examples:
            | profile | highway     | contraflow | first | second |
            | bicycle | residential |            | 1     | 2      |
            | bicycle | residential | no         | 2     | 3      |
            | bicycle | service     |            | 1     | 2      |
            | car     | residential |            | 1     | 2      |

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
            | waypoints | route       | turns                                                         | modes                                     | exit_numbers |
            | a,d       | ab,dc,dc,dc | depart,roundabout-exit-undefined,exit roundabout right,arrive | cycling,cycling,pushing bike,pushing bike | -,-,-,-      |
            | 1,d       | ,dc,dc      | depart,exit roundabout right,arrive                           | cycling,pushing bike,pushing bike         | -,-,-        |
            | 1,f       | ,ef,ef      | depart,exit roundabout slight right,arrive                    | cycling,cycling,cycling                   | -,1,-        |
            | a,1       | ab,,        | depart,roundabout-exit-undefined,arrive                       | cycling,cycling,cycling                   | -,-,-        |

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
            | waypoints | route          | turns                                                    | modes                             | exit_numbers |
            | a,l       | ab,jkl,jkl,jkl | depart,roundabout-exit-1,exit roundabout straight,arrive | cycling,cycling,cycling,cycling   | -,1,1,-      |
            | a,c       | ab,cb,cb       | depart,exit roundabout right,arrive                      | cycling,pushing bike,pushing bike | -,-,-        |

    Scenario: Count the rideable exit when the same junction has a pushing-only arm
        Given the profile "bicycle"
        Given the node map
            """
                a
                b
            d c   g h
              j e
                f
            """

        And the ways
            | nodes | name | highway     | junction   | oneway |
            | ab    | ab   | residential |            | yes    |
            | dc    | dc   | residential |            | yes    |
            | cj    | cj   | residential |            | yes    |
            | fe    | fe   | residential |            | yes    |
            | gh    | gh   | residential |            | yes    |
            | bcegb |      | residential | roundabout |        |

        When I route I should get
            | waypoints | route       | turns                                                         | modes                                     | exit_numbers |
            | a,j       | ab,cj,cj,cj | depart,roundabout-exit-1,exit roundabout left,arrive          | cycling,cycling,cycling,cycling           | -,1,1,-      |
            | a,f       | ab,fe,fe,fe | depart,roundabout-exit-undefined,exit roundabout right,arrive | cycling,cycling,pushing bike,pushing bike | -,-,-,-      |
            | a,h       | ab,gh,gh,gh | depart,roundabout-exit-2,exit roundabout right,arrive         | cycling,cycling,cycling,cycling           | -,2,2,-      |

    Scenario: Skip an inbound arm on a clockwise roundabout
        Given the profile file
            """
            local functions = require("bicycle")
            local process_way = functions.process_way
            functions.process_way = function(profile, way, result, relations)
                process_way(profile, way, result, relations)
                result.is_left_hand_driving = true
            end
            return functions
            """

        And the node map
            """
                a
                b
            h g   c d
                e
                f
            """

        And the ways
            | nodes | name | highway     | junction   | oneway |
            | ab    | ab   | residential |            | yes    |
            | cd    | cd   | residential |            | -1     |
            | ef    | ef   | residential |            | yes    |
            | gh    | gh   | residential |            | yes    |
            | bcegb |      | residential | roundabout |        |

        When I route I should get
            | waypoints | route       | turns                                                        | driving_side        | exit_numbers |
            | a,d       | ab,cd,cd,cd | depart,roundabout-exit-undefined,exit roundabout left,arrive | left,left,left,left | -,-,-,-      |
            | a,f       | ab,ef,ef,ef | depart,roundabout-exit-1,exit roundabout left,arrive         | left,left,left,left | -,1,1,-      |
            | a,h       | ab,gh,gh,gh | depart,roundabout-exit-2,exit roundabout left,arrive         | left,left,left,left | -,2,2,-      |
