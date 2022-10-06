@routing @car @way
Feature: Car - Avoid defined areas

    Background:
        Given the profile file "car" initialized with
        """
        profile.avoid = Set { 'motorway', 'motorway_link' }
        profile.speeds = Sequence {
            highway = {
                motorway      = 90,
                motorway_link = 45,
                primary       = 50
            }
        }
        """

    Scenario: Car - Avoid motorways
        Then routability should be
            | highway        | bothw |
            | motorway       |       |
            | motorway_link  |       |
            | primary        | x     |

