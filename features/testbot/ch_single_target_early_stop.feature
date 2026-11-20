@matrix @testbot
Feature: Single-target CH table early termination

    Background:
        Given the profile "testbot"

    Scenario: Existing turn-duration alternatives remain stable in a single-target search
        Given the profile file
        """
        local functions = require('testbot')
        functions.process_segment = function(profile, segment)
          segment.weight = 1
          segment.duration = 1
        end
        functions.process_turn = function(profile, turn)
          turn.duration = turn.angle >= 0 and 16 or 4
          turn.weight = 0
        end
        return functions
        """
        And the node map
            """
            a     b

            c     d
            """
        And the ways
            | nodes |
            | ab    |
            | ac    |
            | bd    |
            | dc    |

        When I request a travel time matrix I should get
            |   | a |
            | a | 0 |
            | b | 1 |
            | c | 1 |
            | d | 6 |

    Scenario: Asymmetric distances remain stable in a single-target search
        Given the node map
            """
            a        b

            c     d
            """
        And the ways
            | nodes |
            | ab    |
            | ac    |
            | bd    |
            | dc    |

        When I request a travel distance matrix I should get
            |   | a   |
            | a | 0   |
            | b | 450 |
            | c | 200 |
            | d | 500 |

    Scenario: A single target behind a source on a one-way loop needs the detour
        Given the node map
            """
            a 1 2 b
            d 4 3 c
            """
        And the ways
            | nodes | oneway |
            | ab    | yes    |
            | bc    | yes    |
            | cd    | yes    |
            | da    | yes    |

        When I request a travel time matrix I should get
            |   | 1      |
            | 1 | 0      |
            | 2 | 70 +-1 |
            | 3 | 40 +-1 |
            | 4 | 30 +-1 |

    Scenario: Duplicate coordinates and source rows do not share completion state
        Given the profile file
        """
        local functions = require('testbot')
        functions.process_segment = function(profile, segment)
          segment.weight = 1
          segment.duration = 1
        end
        functions.process_turn = function(profile, turn)
          turn.weight = 0
          turn.duration = 0
        end
        return functions
        """
        And the node locations
            | node | lon   | lat |
            | a    | 0     | 1   |
            | b    | 0.001 | 1   |
            | c    | 0.002 | 1   |
            | 1    | 0     | 1   |
            | 2    | 0     | 1   |
        And the ways
            | nodes |
            | abc   |

        When I request a travel time matrix I should get
            |   | c |
            | a | 2 |
            | b | 1 |
            | 1 | 2 |
            | 2 | 2 |
            | c | 0 |

    Scenario: A finite row does not make an unreachable row complete early
        Given the profile file
        """
        local functions = require('testbot')
        functions.process_segment = function(profile, segment)
          segment.weight = 1
          segment.duration = 1
        end
        functions.process_turn = function(profile, turn)
          turn.weight = 0
          turn.duration = 0
        end
        return functions
        """
        And the node map
            """
            a b c
            """
        And the ways
            | nodes | oneway |
            | abc   | yes    |

        When I request a travel time matrix I should get
            |   | b |
            | a | 1 |
            | b | 0 |
            | c |   |

        When I request a travel time matrix I should get
            |   | a |
            | a | 0 |
            | b |   |
            | c |   |

    Scenario: Negative turn durations retain the unbounded reference search
        Given the profile file
        """
        local functions = require('testbot')
        local base_setup = functions.setup
        functions.setup = function()
          local profile = base_setup()
          profile.properties.weight_name = 'test_weight'
          return profile
        end
        functions.process_way = function(profile, way, result)
          result.forward_mode = mode.driving
          result.backward_mode = mode.inaccessible
          result.weight = 1
          result.duration = 1
        end
        functions.process_turn = function(profile, turn)
          turn.weight = 0
          turn.duration = -2
        end
        return functions
        """
        And the node map
            """
            a b c
              d
            """
        And the ways
            | nodes | oneway |
            | ab    | yes    |
            | bc    | yes    |
            | db    | yes    |

        When I request a travel time matrix I should get
            |   | c |
            | a | 0 |
            | d | 0 |
            | c | 0 |

    Scenario: Equal-cost distance ties keep the reference settlement order
        Given the profile file
        """
        local functions = require('testbot')
        local base_setup = functions.setup
        functions.setup = function()
          local profile = base_setup()
          profile.properties.weight_name = 'test_weight'
          profile.properties.weight_precision = 1
          return profile
        end
        functions.process_way = function(profile, way, result)
          result.forward_mode = mode.driving
          result.backward_mode = mode.driving
          result.weight = 1
          result.duration = 1
        end
        functions.process_turn = function(profile, turn)
          turn.weight = turn.is_u_turn and 100 or 0
          turn.duration = turn.is_u_turn and 100 or 0
        end
        return functions
        """
        And the node locations
            | node | lon      | lat   |
            | a    | 0        | 1     |
            | b    | 0.001    | 1     |
            | c    | 0        | 1.001 |
            | d    | 0.001001 | 1.001 |
        And the ways
            | nodes |
            | ab    |
            | ac    |
            | bd    |
            | cd    |

        When I request a travel time matrix I should get
            |   | d |
            | a | 2 |
            | b | 1 |
            | c | 1 |
            | d | 0 |

        When I request a travel distance matrix I should get
            |   | d        |
            | a | 222 +-1  |
            | b | 111 +-1  |
            | c | 111 +-1  |
            | d | 0        |
