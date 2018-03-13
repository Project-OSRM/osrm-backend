@testbot
Feature: Weight tests

    Background:
        Given the profile "testbot"
        Given a grid size of 10 meters
        Given the query options
            | geometries | geojson |

    Scenario: Weight details
        Given the query options
            | annotations | weight |

        Given the node map
            """
              s
              ·
            a---b---c
                    |
                    d
                    |··t
                    e
            """

        And the ways
            | nodes |
            | abc   |
            | cde   |

        When I route I should get
            | waypoints | route   | a:weight  |
            | s,t       | abc,cde | 1.1:2:2:1 |

        When I route I should get
            | waypoints | route   | times   | weight_name | weights |
            | s,t       | abc,cde | 6.1s,0s | duration    | 6.1,0   |

    # FIXME include/engine/guidance/assemble_geometry.hpp:95
    Scenario: Start and target on the same and adjacent edge
        Given the query options
            | annotations | distance,duration,weight,nodes,speed |

        Given the node map
            """
            a-------b-------c
              ·   ·   ·
              s   t   e
            """

        And the ways
            | nodes |
            | abc   |

        When I route I should get
            | waypoints | route   | distances | weights | times   | a:distance          | a:duration | a:weight | a:speed |
            | s,t       | abc,abc | 20m,0m    | 2.1,0   | 2.1s,0s | 20.017685           | 2.1        | 2.1      | 9.5     |
            | t,s       | abc,abc | 20m,0m    | 2.1,0   | 2.1s,0s | 20.017685           | 2.1        | 2.1      | 9.5     |
            | s,e       | abc,abc | 40m,0m    | 4.1,0   | 4.1s,0s | 30.026527:10.008842 | 3.1:1      | 3.1:1    | 9.7:10  |
            | e,s       | abc,abc | 40m,0m    | 4.1,0   | 4.1s,0s | 10.008842:30.026527 | 1:3.1      | 1:3.1    | 10:9.7  |


    Scenario: Step weights -- way_function: fail if no weight or weight_per_meter property
        Given the profile file
        """
        local functions = require('testbot')
        functions.setup_testbot = functions.setup

        functions.setup = function()
          local profile = functions.setup_testbot()
          profile.properties.traffic_signal_penalty = 0
          profile.properties.u_turn_penalty = 0
          profile.properties.weight_name = 'steps'
          return profile
        end

        functions.process_way = function(profile, way, result)
          result.forward_mode = mode.driving
          result.backward_mode = mode.driving
          result.forward_speed = 42
          result.backward_speed = 42
        end

        return functions
        """
        And the node map
            """
            a---b
            """
        And the ways
            | nodes |
            | ab    |
        And the data has been saved to disk

        When I try to run "osrm-extract {osm_file} --profile {profile_file}"
        Then stderr should contain "There are no edges"
        And it should exit with an error

    Scenario: Step weights -- way_function: second way wins
        Given the profile file
        """
        local functions = require('testbot')
        functions.setup_testbot = functions.setup

        functions.setup = function()
          local profile = functions.setup_testbot()
          profile.properties.traffic_signal_penalty = 0
          profile.properties.u_turn_penalty = 0
          profile.properties.weight_name = 'steps'
          return profile
        end

        functions.process_way = function(profile, way, result)
          result.forward_mode = mode.driving
          result.backward_mode = mode.driving
          result.duration = 42
          result.weight = 35
        end

        return functions
        """

        Given the node map
            """
            a---b---c---d---e---f---g---h
            """

        And the ways
            | nodes    |
            | abcdef   |
            | abcdefgh |

        When I route I should get
            | waypoints | route | distance | weights | times  |
            | a,f       | ,     | 100m     | 25,0    | 30s,0s |
            | f,a       | ,     | 100m     | 25,0    | 30s,0s |
            | a,h       | ,     | 140m +-1 | 35,0    | 42s,0s |
            | h,a       | ,     | 140m +-1 | 35,0    | 42s,0s |

    Scenario: Step weights -- way_function: higher weight_per_meter is preferred
        Given the profile file
        """
        local functions = require('testbot')
        functions.setup_testbot = functions.setup

        functions.setup = function()
          local profile = functions.setup_testbot()
          profile.properties.traffic_signal_penalty = 0
          profile.properties.u_turn_penalty = 0
          profile.properties.weight_name = 'steps'
          return profile
        end

        functions.process_way = function(profile, way, result)
          result.forward_mode = mode.driving
          result.backward_mode = mode.driving
          result.duration = 42
          result.forward_rate = 1
          result.backward_rate = 0.5
        end

        return functions
        """

        Given the node map
            """
            a---b---c---d---e---f---g---h
            """

        And the ways
            | nodes    |
            | abcdefgh |
            | abcdef   |
            | fgh      |

        When I route I should get
            | waypoints | route | distance | weights | times  |
            | a,f       | ,     | 100m     | 99.9,0  | 30s,0s |
            | f,a       | ,     | 100m     | 199.8,0 | 30s,0s |
            | a,h       | ,     | 140m     | 139.9,0 | 42s,0s |
            | h,a       | ,     | 140m     | 279.8,0 | 42s,0s |
            | f,h       | ,     | 40m      | 40,0    | 12s,0s |
            | h,f       | ,     | 40m      | 80,0    | 12s,0s |

    Scenario: Step weights -- segment_function
        Given the profile file
        """
        local functions = require('testbot')
        functions.setup_testbot = functions.setup

        functions.setup = function()
          local profile = functions.setup_testbot()
          profile.properties.traffic_signal_penalty = 0
          profile.properties.u_turn_penalty = 0
          profile.properties.weight_name = 'steps'
          return profile
        end

        functions.process_way = function(profile, way, result)
          result.forward_mode = mode.driving
          result.backward_mode = mode.driving
          result.weight = 42
          result.duration = 3
        end

        functions.process_segment = function(profile, segment)
          segment.weight = 1
          segment.duration = 11
        end

        return functions
        """

        Given the node map
            """
            a---b---c---d---e---f---g---h
            """

        And the ways
            | nodes    |
            | abcdefgh |
            | abcdef   |
            | fgh      |

        When I route I should get
            | waypoints | route | distance | weights | times  |
            | a,f       | ,     | 100m     | 5,0     | 55s,0s |
            | f,a       | ,     | 100m     | 5,0     | 55s,0s |
            | a,h       | ,     | 140m +-1 | 7,0     | 77s,0s |
            | h,a       | ,     | 140m +-1 | 7,0     | 77s,0s |
            | f,h       | ,     | 40m +-1  | 2,0     | 22s,0s |
            | h,f       | ,     | 40m +-1  | 2,0     | 22s,0s |


    Scenario: Step weights -- segment_function and turn_function with weight precision
        Given the profile file
        """
        local functions = require('testbot')
        functions.setup_testbot = functions.setup

        functions.setup = function()
          local profile = functions.setup_testbot()
          profile.properties.traffic_signal_penalty = 0
          profile.properties.u_turn_penalty = 0
          profile.properties.weight_name = 'steps'
          profile.properties.weight_precision = 3
          return profile
        end

        functions.process_way = function(profile, way, result)
          result.forward_mode = mode.driving
          result.backward_mode = mode.driving
          result.weight = 42
          result.duration = 3
        end

        functions.process_segment = function(profile, segment)
          segment.weight = 1.11
          segment.duration = 100
        end

        functions.process_turn = function(profile, turn)
          print (turn.angle)
          turn.weight = 2 + turn.angle / 100
          turn.duration = turn.angle
        end

        return functions
        """

        Given the node map
            """
            a---b---c---d
                    ⋮
                    e
            """

        And the ways
            | nodes |
            | abcd  |
            | ce    |

        When I route I should get
            | waypoints | route | distance | weights      | times          |
            | a,c       | ,     | 40m +-.1 | 5.119,0      | 289.9s,0s      |
            | a,e       | ,,    | 60m +-.1 | 5.119,1.11,0 | 289.9s,100s,0s |
            | e,a       | ,,    | 60m +-.1 | 2.21,2.22,0  | 10.1s,200s,0s  |
            | e,d       | ,,    | 40m +-.1 | 4.009,1.11,0 | 189.9s,100s,0s |
            | d,e       | ,,    | 40m +-.1 | 2.21,1.11,0  | 10.1s,100s,0s  |

    @traffic @speed
    Scenario: Step weights -- segment_function with speed and turn updates
        Given the profile file
        """
        local functions = require('testbot')
        functions.setup_testbot = functions.setup

        functions.setup = function()
          local profile = functions.setup_testbot()
          profile.properties.traffic_signal_penalty = 0
          profile.properties.u_turn_penalty = 0
          profile.properties.weight_name = 'steps'
          return profile
        end

        functions.process_way = function(profile, way, result)
          result.forward_mode = mode.driving
          result.backward_mode = mode.driving
          result.weight = 42
          result.duration = 3
        end

        functions.process_segment = function(profile, segment)
          segment.weight = 10
          segment.duration = 11
        end

        return functions
        """

        And the node map
            """
            a---b---c---d
                    .
                    e
            """
        And the ways
            | nodes |
            | abcd  |
            | ce    |
        And the speed file
            """
            1,2,36,42
            2,1,36,42
            """
        And the turn penalty file
            """
            2,3,5,25.5,16.7
            """
        And the contract extra arguments "--segment-speed-file {speeds_file} --turn-penalty-file {penalties_file}"
        And the customize extra arguments "--segment-speed-file {speeds_file} --turn-penalty-file {penalties_file}"

        When I route I should get
            | waypoints | route | distance | weights   | times        |
            | a,d       | ,     | 59.9m    | 20.5,0    | 24s,0s       |
            | a,e       | ,,    | 60.1m    | 27.2,10,0 | 38.5s,11s,0s |
            | d,e       | ,,    | 39.9m    | 10,10,0   | 11s,11s,0s   |

    @traffic @speed
    Scenario: Step weights -- segment_function with speed and turn updates with fallback to durations
        Given the profile file "testbot" initialized with
        """
        profile.properties.weight_precision = 3
        """

        And the node map
            """
            a---b---c---d
                    .
                    e
            """
        And the ways
            | nodes |
            | abcd  |
            | ce    |
        And the speed file
            """
            1,2,24
            2,1,24
            """
        And the turn penalty file
            """
            2,3,5,1
            """
        And the contract extra arguments "--segment-speed-file {speeds_file} --turn-penalty-file {penalties_file}"
        And the customize extra arguments "--segment-speed-file {speeds_file} --turn-penalty-file {penalties_file}"

        When I route I should get
            | waypoints | route      | distance | weights       | times    |
            | a,d       | abcd,abcd  | 59.9m    | 6.996,0       | 7s,0s    |
            | a,e       | abcd,ce,ce | 60.1m    | 6.005,2.002,0 | 6s,2s,0s |
            | d,e       | abcd,ce,ce | 39.9m    | 1.991,2.002,0 | 2s,2s,0s |

    @traffic @speed
    Scenario: Updating speeds without affecting weights.
        Given the profile file "testbot" initialized with
        """
        profile.properties.weight_precision = 3
        """

        And the node map
            """
            a-----------b
             \         /
                c----d
            """
        And the ways
            | nodes | highway       | maxspeed |
            | ab    | living_street | 5        |
            | acdb  | motorway      | 100      |

        # Note the comma on the last column - this indicates 'keep existing weight value'
        And the speed file
            """
            1,2,100,
            1,3,5,,junk
            3,4,5,,
            4,2,5,
            """
        And the contract extra arguments "--segment-speed-file {speeds_file}"
        And the customize extra arguments "--segment-speed-file {speeds_file}"

        When I route I should get
            | waypoints | route      | distance | weights       | times    |
            | a,b       | acdb,acdb  | 78.3m    | 11.744,0      | 56.4s,0s  |
