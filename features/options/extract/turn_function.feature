@routing @testbot @turn_function
Feature: Turn Function Information


    Background:
        Given the profile file
        """
        functions = require('car')

        function test_setup()
          profile = functions.setup()
          profile.highway_turn_classification = {
              ['motorway'] = 4,
              ['motorway_link'] = 4,
              ['trunk'] = 4,
              ['trunk_link'] = 4,
              ['primary'] = 4,
              ['primary_link'] = 4,
              ['secondary'] = 3,
              ['secondary_link'] = 3,
              ['tertiary'] = 2,
              ['tertiary_link'] = 2,
              ['residential'] = 1,
              ['living_street'] = 1,
          }

          profile.access_turn_classification = {
              ['discouraged'] = 1;
              ['permissive'] = 1;
              ['private'] = 1;
              ['customers'] = 1;
              ['dismount'] = 1;
          }
          return profile
        end

        function turn_leg_string (leg)
          return 'speed: ' .. tostring(leg.speed)
             .. ', is_incoming: ' .. tostring(leg.is_incoming)
             .. ', is_outgoing: ' .. tostring(leg.is_outgoing)
             .. ', highway_turn_classification: ' .. tostring(leg.highway_turn_classification)
             .. ', access_turn_classification: ' .. tostring(leg.access_turn_classification)
             .. ', priority_class: ' .. tostring(leg.priority_class)
        end

        function print_turn (profile, turn)
          print ('source_restricted ' .. string.format("%s", tostring(turn.source_restricted)))
          print ('source_is_motorway ' .. string.format("%s", tostring(turn.source_is_motorway)))
          print ('source_is_link ' .. string.format("%s", tostring(turn.source_is_link)))
          print ('source_number_of_lanes ' .. string.format("%s", tostring(turn.source_number_of_lanes)))
          print ('source_highway_turn_classification ' .. string.format("%s", tostring(turn.source_highway_turn_classification)))
          print ('source_access_turn_classification ' .. string.format("%s", tostring(turn.source_access_turn_classification)))
          print ('source_speed ' .. string.format("%s", tostring(turn.source_speed)))
          print ('source_priority_class ' .. string.format("%s", tostring(turn.source_priority_class)))
          print ('source_mode ' .. string.format("%s", tostring(turn.source_mode)))

          print ('target_restricted ' .. string.format("%s", tostring(turn.target_restricted)))
          print ('target_is_motorway ' .. string.format("%s", tostring(turn.target_is_motorway)))
          print ('target_is_link ' .. string.format("%s", tostring(turn.target_is_link)))
          print ('target_number_of_lanes ' .. string.format("%s", tostring(turn.target_number_of_lanes)))
          print ('target_highway_turn_classification ' .. string.format("%s", tostring(turn.target_highway_turn_classification)))
          print ('target_access_turn_classification ' .. string.format("%s", tostring(turn.target_access_turn_classification)))
          print ('target_speed ' .. string.format("%s", tostring(turn.target_speed)))
          print ('target_priority_class ' .. string.format("%s", tostring(turn.target_priority_class)))
          print ('target_mode ' .. string.format("%s", tostring(turn.target_mode)))

          print ('number_of_roads ' .. string.format("%s", tostring(turn.number_of_roads)))
          if not turn.is_u_turn then
          for roadCount, road in ipairs(turn.roads_on_the_right) do
              print('roads_on_the_right [' .. tostring(roadCount) .. '] ' .. turn_leg_string(road))
            end

            for roadCount, road in ipairs(turn.roads_on_the_left) do
              print('roads_on_the_left [' .. tostring(roadCount) .. '] ' .. turn_leg_string(road))
            end
          end
        end

        return {
            setup = test_setup,
            process_way = functions.process_way,
            process_node = functions.process_node,
            process_turn = print_turn
        }
        """

    Scenario: Turns should have correct information of source and target
        Given the node map
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
        And stdout should contain "source_priority_class 0"
        And stdout should contain "target_is_motorway true"
        And stdout should contain "target_is_link false"
        And stdout should contain "target_priority_class 0"


    Scenario: Turns should detect when turn is leaving highway
        Given the node map
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
        And stdout should contain "source_number_of_lanes 3"
        And stdout should contain "target_is_motorway false"
        And stdout should contain "target_is_link true"
        And stdout should contain "target_number_of_lanes 0"
        And stdout should contain "number_of_roads 2"

    Scenario: Turns should have correct information of other roads at intersection I
        Given the node map
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
        And stdout should contain "source_priority_class 4"
        And stdout should contain "target_priority_class 0"
        And stdout should contain "target_priority_class 10"
        # turning abd, give information about bc
        And stdout should contain /roads_on_the_right \[1\] speed: [0-9]+, is_incoming: false, is_outgoing: true, highway_turn_classification: 4, access_turn_classification: 0/
        # turning abc, give information about bd
        And stdout should contain /roads_on_the_left \[1\] speed: [0-9]+, is_incoming: false, is_outgoing: true, highway_turn_classification: 1, access_turn_classification: 0/

    Scenario: Turns should have correct information of other roads at intersection II
        Given the node map
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
        And stdout should contain /roads_on_the_right \[1\] speed: [0-9]+, is_incoming: true, is_outgoing: false, highway_turn_classification: 3, access_turn_classification: 0/
        # turning abc, give information about about db
        And stdout should contain /roads_on_the_left \[1\] speed: [0-9]+, is_incoming: true, is_outgoing: false, highway_turn_classification: 0, access_turn_classification: 1/
