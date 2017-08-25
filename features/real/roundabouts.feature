@routing @car @bugs
Feature: Real test scenarios from OSM

    Background:
        Given the profile "car"

    Scenario: Routing across an untagged roundabout
        Given the input file test/data/extracts/untagged_roundabout.osm

        When I route I should get
            | from                | to                 | turns                    | route                                                      |
            | -77.016381,38.91594 | 38.91591,-77.01524 | depart,turn right,arrive | 3rd Street Northwest,T Street Northwest,T Street Northwest |
