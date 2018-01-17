@routing @testbot @turn_function
Feature: Turn Function Information

    Scenario: Turns should have correct information of source and target
        Given the profile file
        """
        functions = require('feature/car_turnbot')

        return {
          setup = functions.setup,
          process_way = functions.process_way,
          process_node = functions.process_node,
          process_turn = functions.process_turn
        }

        """

        And the node map
            """

            a b c

            """
        And the ways
            | nodes | highway  |
            | ab    | motorway |
            | bc    | motorway |
        And the data has been saved to disk

        When I run "osrm-extract --profile {profile_file} {osm_file}"
        Then it should exit successfully
        And stdout should contain "source_is_motorway true"
        And stdout should contain "target_is_motorway true"
        And stdout should contain "source_is_link false"
        And stdout should contain "source_speed 90"
        And stdout should contain "target_is_motorway true"
        And stdout should contain "target_is_link false"
        And stdout should contain "target_speed 90"


    Scenario: Turns should detect when turn is leaving highway
        Given the profile file
        """
        functions = require('feature/car_turnbot')

        return {
          setup = functions.setup,
          process_way = functions.process_way,
          process_node = functions.process_node,
          process_turn = functions.process_turn
        }


        """

        And the node map
            """

            a b c

            """
        And the ways
            | nodes | highway       | lanes |
            | ab    | motorway      | 3     |
            | bc    | motorway_link |       |

        And the data has been saved to disk

        When I run "osrm-extract --profile {profile_file} {osm_file}"
        Then it should exit successfully
        And stdout should contain "source_is_motorway true"
        And stdout should contain "source_is_link false"
        And stdout should contain "source_speed 90"
        And stdout should contain "source_number_of_lanes 3"
        And stdout should contain "target_is_motorway false"
        And stdout should contain "target_is_link true"
        # Question @review: should number_of_lanes when untagged be 0 or 1?
        And stdout should contain "target_number_of_lanes 0"
        And stdout should contain "number_of_roads 2"

    Scenario: Turns should have correct information of other roads at intersection I
        Given the profile file
        """
        functions = require('feature/car_turnbot')

        return {
          setup = functions.setup,
          process_way = functions.process_way,
          process_node = functions.process_node,
          process_turn = functions.process_turn
        }

        """

        And the node map
            """
               d
               ^
               |
            a->b->c
            """
        And the ways
            | nodes | highway     | oneway |
            | ab    | primary     | yes    |
            | bc    | motorway    | yes    |
            | bd    | residential | yes    |
        And the data has been saved to disk

        When I run "osrm-extract --profile {profile_file} {osm_file}"
        Then it should exit successfully
        And stdout should contain "number_of_roads 3"
        # turning abd, give information about bc
        And stdout should contain "roads_on_the_right [1] speed: 90, is_incoming: false, is_outgoing: true, highway_turn_classification: 4, access_turn_classification: 0"
        # turning abc, give information about bd
        And stdout should contain "roads_on_the_left [1] speed: 25, is_incoming: false, is_outgoing: true, highway_turn_classification: 1, access_turn_classification: 0"

    Scenario: Turns should have correct information of other roads at intersection II
        Given the profile file
        """
        functions = require('feature/car_turnbot')

        return {
          setup = functions.setup,
          process_way = functions.process_way,
          process_node = functions.process_node,
          process_turn = functions.process_turn
        }

        """

        And the node map
            """
               d
               |
               v
            a->b->c
            """
        And the ways
            | nodes | highway      | oneway | access      |
            | ab    | secondary    | yes    |             |
            | bc    | motorway     | yes    |             |
            | db    | unclassified | yes    | discouraged |
        And the data has been saved to disk

        When I run "osrm-extract --profile {profile_file} {osm_file}"
        Then it should exit successfully
        And stdout should contain "number_of_roads 3"
        # turning dbc, give information about about ab
        And stdout should contain "roads_on_the_right [1] speed: 55, is_incoming: true, is_outgoing: false, highway_turn_classification: 3, access_turn_classification: 0"
        # turning abc, give information about about db
        And stdout should contain "roads_on_the_left [1] speed: 25, is_incoming: true, is_outgoing: false, highway_turn_classification: 0, access_turn_classification: 1"




