@routing @isochrone @testbot
Feature: Isochrone service

    Background:
        Given the profile "testbot"
        And a grid size of 100 meters
        And the partition extra arguments "--generate-isochrone-data"
        And the customize extra arguments "--generate-isochrone-data"
        And the contract extra arguments "--generate-isochrone-data"

    Scenario: Return one polygon feature for each requested contour
        Given the node map
            """
            a b c
            d e f
            """
        And the ways
            | nodes |
            | abc   |
            | def   |
            | ad    |
            | be    |
            | cf    |

        When I request an isochrone from "e" with contours "10,20"
        Then the isochrone response should be a GeoJSON FeatureCollection with "2" "MultiPolygon" features
        And the isochrone contours should be "10,20"
        And the isochrone response should have "1" waypoint

    Scenario: Use duration contours when primary routing weight differs
        Given a grid size of 500 meters
        And the profile file
            """
            local functions = require('testbot')
            functions.setup_testbot = functions.setup

            functions.setup = function()
              local profile = functions.setup_testbot()
              profile.properties.weight_name = 'steps'
              profile.properties.traffic_signal_penalty = 0
              profile.properties.u_turn_penalty = 0
              return profile
            end

            functions.process_way = function(profile, way, result)
              result.forward_mode = mode.driving
              result.backward_mode = mode.driving
              result.duration = tonumber(way:get_value_by_key('duration'))
              result.weight = tonumber(way:get_value_by_key('weight'))
            end

            return functions
            """
        And the node map
            """
            a b c
            """
        And the ways
            | nodes | duration | weight |
            | ab    | 5        | 100    |
            | bc    | 20       | 1      |

        When I request an isochrone from "a" with contours "10"
        Then the isochrone response should be a GeoJSON FeatureCollection with "1" "MultiPolygon" features
        And the isochrone should contain "b" and not contain "c"

    Scenario: Follow profile-weight-optimal paths when weights differ from durations
        Given a grid size of 500 meters
        And the profile file
            """
            local functions = require('testbot')
            functions.setup_testbot = functions.setup

            functions.setup = function()
              local profile = functions.setup_testbot()
              profile.properties.weight_name = 'steps'
              profile.properties.traffic_signal_penalty = 0
              profile.properties.u_turn_penalty = 0
              return profile
            end

            functions.process_way = function(profile, way, result)
              result.forward_mode = mode.driving
              result.backward_mode = mode.driving
              result.name = way:get_value_by_key('name')
              result.duration = tonumber(way:get_value_by_key('duration'))
              result.weight = tonumber(way:get_value_by_key('weight'))
            end

            return functions
            """
        And the node map
            """
            a x
             y m t
            """
        And the ways
            | nodes | duration | weight |
            | axm   | 5        | 100    |
            | aym   | 15       | 1      |
            | mt    | 5        | 1      |

        # The path via x reaches t in 10 seconds but has weight 101. Route and
        # table select the 20-second path via y because its weight is 2.
        When I route I should get
            | from | to | route   | time | weight |
            | a    | t  | aym,mt,mt | 20s  | 2      |
        When I request an isochrone from "a" with contours "10"
        Then the isochrone response should be a GeoJSON FeatureCollection with "1" "MultiPolygon" features
        And the isochrone should contain "y" and not contain "t"
        When I request a travel time matrix I should get
            |   | a | t  |
            | a | 0 | 20 |

    Scenario: Use durations updated by a segment speed file
        Given the node locations
            | node | lon   | lat | id |
            | a    | 1     | 1   | 1  |
            | b    | 1.005 | 1   | 2  |
        And the ways
            | nodes |
            | ab    |
        And the contract extra arguments "--generate-isochrone-data --segment-speed-file {speeds_file}"
        And the customize extra arguments "--generate-isochrone-data --segment-speed-file {speeds_file}"
        And the speed file
            """
            1,2,1
            2,1,1
            """

        When I request an isochrone from "a" with contours "100"
        Then the isochrone response should be a GeoJSON FeatureCollection with "1" "MultiPolygon" features
        And the isochrone should contain "a" and not contain "b"

    Scenario: Return contour boundaries as lines
        Given the query options
            | polygons | false |
        And the node map
            """
            a b c
            """
        And the ways
            | nodes |
            | abc   |

        When I request an isochrone from "b" with contours "10"
        Then the isochrone response should be a GeoJSON FeatureCollection with "1" "MultiLineString" features
        And the isochrone contours should be "10"

    Scenario: Omit waypoints when requested
        Given the query options
            | skip_waypoints | true |
        And the node map
            """
            a b
            """
        And the ways
            | nodes |
            | ab    |

        When I request an isochrone from "a" with contours "20"
        Then the isochrone response should be a GeoJSON FeatureCollection with "1" "MultiPolygon" features
        And the isochrone response should omit waypoints

    Scenario: Respect one-way direction for inbound and outbound searches
        Given a grid size of 500 meters
        Given the node map
            """
            a 1 b
            d   c
            """
        And the ways
            | nodes | oneway |
            | ab    | yes    |
            | bc    | yes    |
            | cd    | yes    |
            | da    | yes    |

        When I request outbound and inbound isochrones from "1" with contour "75"
        Then both directional isochrone responses should be GeoJSON FeatureCollections
        And the outbound and inbound isochrone geometries should differ
        And the outbound isochrone should contain "b" and the inbound isochrone should not

    Scenario: Return an empty area when no directed edge can be departed
        Given the node map
            """
            a b
            """
        And the ways
            | nodes | oneway |
            | ba    | yes    |

        When I request an isochrone from "a" with contours "20"
        Then the isochrone response should contain an empty area

    Scenario: Leave feature-disabled datasets compatible
        Given the partition extra arguments "--threads 1"
        And the contract extra arguments "--threads 1"
        And the customize extra arguments "--threads 1"
        And the node map
            """
            a b
            """
        And the ways
            | nodes |
            | ab    |

        When I route I should get
            | from | to | route |
            | a    | b  | ab,ab |
        When I request /isochrone/v1/testbot/1,1?contours=20
        Then the isochrone HTTP status should be 400
        And status code should be NotImplemented

    Scenario: Respect excluded road classes
        Given a grid size of 500 meters
        And the query options
            | exclude | motorway |
        And the node map
            """
            a b c
            """
        And the ways
            | nodes | highway  |
            | ab    | primary  |
            | bc    | motorway |

        When I request an isochrone from "a" with contours "200"
        Then the isochrone should contain "b" and not contain "c"

    Scenario: Exclude a source-adjacent motorway bridge
        Given a grid size of 500 meters
        And the query options
            | exclude | motorway |
        And the node map
            """
            a b c
            |
            d
            """
        And the ways
            | nodes | highway  |
            | ab    | motorway |
            | bc    | primary  |
            | ad    | primary  |

        # The excluded first hop is the only bridge to b and c. The source's
        # primary branch must still be reachable through the selected facade.
        When I request an isochrone from "a" with contours "200"
        Then the isochrone should contain "d" and not contain "b"

    Scenario: Reject geometry crossing the antimeridian
        Given the node locations
            | node | lon  | lat |
            | a    | 179  | 0   |
            | b    | -179 | 0   |
        And the ways
            | nodes |
            | ab    |

        When I request /isochrone/v1/testbot/179,0?contours=10
        Then the isochrone HTTP status should be 400
        And status code should be NotImplemented

    Scenario: Reject a raster cell beginning at the longitude world boundary
        Given the node locations
            | node | lon     | lat |
            | a    | 180     | 0   |
            | b    | 179.999 | 0   |
        And the ways
            | nodes |
            | ab    |

        When I request /isochrone/v1/testbot/180,0?contours=10
        Then the isochrone HTTP status should be 400
        And status code should be NotImplemented

    Scenario: Reject a raster extent that touches a pole
        Given the node locations
            | node | lon | lat     |
            | a    | 0   | 89.9995 |
            | b    | 0   | 89.998  |
        And the ways
            | nodes |
            | ab    |

        When I request /isochrone/v1/testbot/0,89.9995?contours=10
        Then the isochrone HTTP status should be 400
        And status code should be NotImplemented

    Scenario: Reject invalid and unsupported isochrone requests
        Given the node map
            """
            a b
            """
        And the ways
            | nodes |
            | ab    |

        When I request /isochrone/v1/testbot/1,1
        Then the isochrone HTTP status should be 400
        And status code should be InvalidOptions

        When I request /isochrone/v1/testbot/1,1?contours=0
        Then the isochrone HTTP status should be 400
        And status code should be InvalidOptions

        When I request /isochrone/v1/testbot/1,1?contours=0.01
        Then the isochrone HTTP status should be 400
        And status code should be InvalidValue

        When I request /isochrone/v1/testbot/1,1;1.001,1?contours=10
        Then the isochrone HTTP status should be 400
        And status code should be InvalidOptions

        When I request /isochrone/v1/testbot/1,1?contours=10&direction=sideways
        Then the isochrone HTTP status should be 400
        And status code should be InvalidQuery

        When I request /isochrone/v1/testbot/1,1?contours=10&polygons=maybe
        Then the isochrone HTTP status should be 400
        And status code should be InvalidQuery

        When I request /isochrone/v1/testbot/1,1.0.flatbuffers?contours=10
        Then the isochrone HTTP status should be 400
        And status code should be NotImplemented
