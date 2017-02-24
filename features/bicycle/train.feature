@routing @bicycle @train
Feature: Bike - Handle ferry routes
# Bringing bikes on trains and subways

    Background:
        Given the profile "bicycle"

    Scenario: Bike - Bringing bikes on trains
        Then routability should be
            | highway | railway    | bicycle | bothw   |
            | primary |            |         | cycling |
            | (nil)   | train      |         |         |
            | (nil)   | train      | no      |         |
            | (nil)   | train      | yes     | train   |
            | (nil)   | railway    |         |         |
            | (nil)   | railway    | no      |         |
            | (nil)   | railway    | yes     | train   |
            | (nil)   | subway     |         |         |
            | (nil)   | subway     | no      |         |
            | (nil)   | subway     | yes     | train   |
            | (nil)   | tram       |         |         |
            | (nil)   | tram       | no      |         |
            | (nil)   | tram       | yes     | train   |
            | (nil)   | light_rail |         |         |
            | (nil)   | light_rail | no      |         |
            | (nil)   | light_rail | yes     | train   |
            | (nil)   | monorail   |         |         |
            | (nil)   | monorail   | no      |         |
            | (nil)   | monorail   | yes     | train   |
            | (nil)   | some_tag   |         |         |
            | (nil)   | some_tag   | no      |         |
            | (nil)   | some_tag   | yes     | cycling |

    @construction
    Scenario: Bike - Don't route on railways under construction
        Then routability should be
            | highway | railway      | bicycle | bothw   |
            | primary |              |         | cycling |
            | (nil)   | construction | yes     |         |
