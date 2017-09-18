@routing @bicycle
Feature: Bike - Use distance weight

    Background:
        Given a grid size of 200 meters

    Scenario: Bike - Check distance weight
        Given the profile file
        """
        local functions = require('bicycle')
        functions.setup_testbot = functions.setup

        functions.setup = function()
          local profile = functions.setup_testbot()
          profile.properties.weight_name = 'distance'
          return profile
        end

        return functions
        """

        Given the node map
            """
            a-b-c
            """


        And the ways
            | nodes | highway     |
            | abc   | residential |

        When I route I should get
            | from | to | route   | weight | time | distance |
            | a    | b  | abc,abc |    200 | 48s  | 200m +-1 |
            | a    | c  | abc,abc |    400 | 96s  | 400m +-1 |
