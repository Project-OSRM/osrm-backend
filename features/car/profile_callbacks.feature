@routing @car
Feature: Car - Custom profile callbacks

    Scenario: A custom setup can inspect additional node access tags
        Given the profile file "car" initialized with
        """
        table.insert(profile.access_tags_hierarchy, 1, 'test:access')
        """
        Then routability should be
            | node/test:access | bothw |
            | yes              | x     |
            | no               |       |

    Scenario: A replacement way callback can route ways without stock car tags
        Given the profile file
        """
        local functions = require('car')
        local process_way = functions.process_way
        functions.process_way = function(profile, way, result, relations)
          if way:get_value_by_key('test:drive') == 'yes' then
            result.forward_mode = mode.driving
            result.backward_mode = mode.driving
            result.forward_speed = 40
            result.backward_speed = 40
            result.forward_rate = 10
            result.backward_rate = 10
            result.name = 'shortcut'
            result.is_startpoint = true
          else
            process_way(profile, way, result, relations)
          end
        end
        return functions
        """
        And the node map
        """
        a b c
            d
        """
        And the ways
            | nodes | highway | test:drive | name |
            | ab    | primary |            | ab   |
            | bc    | (nil)   | yes        |      |
            | cd    | primary |            | cd   |
        When I route I should get
            | from | to | route |
            | a    | d  | ab,cd |

    Scenario: A Lua profile can opt in with its own tag keys
        Given the profile file
        """
        local functions = require('car')
        local setup, process_way = functions.setup, functions.process_way
        functions.setup = function()
          return setup()
        end
        functions.process_way = function(profile, way, result, relations)
          local tags = way:snapshot_tags()
          assert(tags['test:road'] ~= nil)
          process_way(profile, way, result, relations)
        end
        functions.tag_filters = {
          [functions.process_way] = {any = {'test:road'}}
        }
        return functions
        """
        And the node map
        """
        a b c
          d
        """
        And the ways
            | nodes | highway | test:road |
            | ab    | primary |           |
            | bc    | primary | yes       |
        And the ways
            | nodes | highway |
            | bd    | primary |
        When I route I should get
            | from | to | route |
            | a    | c  | ab,bc |

    Scenario: A profile importing car handlers still receives native way objects
        Given the profile file
        """
        local functions = require('car')
        local names = WayHandlers.names
        WayHandlers.names = function(profile, way, result, data, relations)
          assert(type(way) == 'userdata')
          names(profile, way, result, data, relations)
          result.name = 'custom'
        end
        return functions
        """
        And the node map
        """
        a b
        """
        And the ways
            | nodes | highway |
            | ab    | primary |
        When I route I should get
            | from | to | route         |
            | a    | b  | custom,custom |

    Scenario Outline: A filter with no required keys uses the original callback
        Given the profile file
        """
        local functions = require('car')
        local setup, process_way = functions.setup, functions.process_way
        functions.setup = function()
          return setup()
        end
        functions.process_node = <node callback>
        functions.process_way = function(profile, way, result, relations)
          process_way(profile, way, result, relations)
        end
        functions.tag_filters = {
          [functions.process_way] = {
            callback = function()
              error('a filter with no keys must use the original callback')
            end,
            <required keys>
          }
        }
        return functions
        """
        And the node map
        """
        a b
        """
        And the ways
            | nodes | highway |
            | ab    | primary |
        When I route I should get
            | from | to | route |
            | a    | b  | ab,ab |

        Examples:
            | node callback  | required keys |
            | nil            | any = {}     |
            | function() end |              |
